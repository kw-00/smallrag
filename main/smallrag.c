#include "smallrag/data_loading.h"
#include "smallrag/normalization.h"
#include "smallrag/dot_product.h"
#include "smallrag/embedder.h"
#include "smallrag/llama_embedder.h"
#include "smallrag/postprocessing_embedder.h"
#include "smallrag/chunker.h"
#include "smallrag/char_chunker.h"
#include "smallrag/logging.h"

#include "llama.h"
#include "cJSON.h"

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mman.h>

/* -m — model path
 * -d — source text path
 * -e — embeddings path
 * -t — number of tokenizer threads
 * -c — context size
 * -b — batch size
 * -u — ubatch size
 * -s — max sequences per batch 
 */
static char *optstring = "mdetcbus";

static size_t parse_size_t_arg()
{
    uintmax_t value = strtoumax(optarg, NULL, 16);
    if (value > SIZE_MAX) {
        perror("strtoumax");
        LOG_ERROR("Number overflow");
        exit(1);
    }
    return value;
}

static void print_required_option_message(char *option_short, char *option_long)
{
    fprintf(stderr, "Option -%s (%s) is required\n", option_short, option_long);
}

static int embedding_postprocessing(struct embeddings *embeddings)
{
    for (size_t i = 0; i < embeddings->embedding_count; i++) {
        if (l2_normalize(embeddings->embeddings + i, embeddings->dimension_count) == -1) {
            LOG_ERROR("Normalization failed");
            return -1;
        }
    }
    return 0;
}

static int compare_similarity(void *size_t index_x, void *size_t index_y, void *similarity_scores)
{
    double *double_scores = similarity_scores;
    double score_x = double_scores[index_x];
    double score_y = double_scores[index_y];
    if (score_x > score_y) {
        return 1;
    } else {
        return -1;
    }
}

int main(int argc, char *argv[])
{
    char *model_path = NULL;
    char *source_text_path = NULL;
    char *embeddings_path = NULL;
    size_t n_tokenizer_threads = 0;
    size_t context_size = 65536;
    size_t batch_size = 2048;
    size_t ubatch_size = 512;
    size_t max_sequences_per_batch = 0;

    while (true) {
        int option = getopt(argc, argv, optstring);
        if (option == -1) break;
        switch (option) {
            case 'm':
                model_path = optarg;
                break;
            case 'd':
                source_text_path = optarg;
                break;
            case 'e':
                embeddings_path = optarg;
                break;
            case 't': 
                n_tokenizer_threads = parse_size_t_arg();
                break;
            case 'c':
                context_size = parse_size_t_arg();
                break;
            case 'b':
                batch_size = parse_size_t_arg();
                break;
            case 'u':
                ubatch_size = parse_size_t_arg();
                break;
            case 's':
                max_sequences_per_batch = parse_size_t_arg();
                break;
        }
    }

    if (model_path == NULL) {
        print_required_option_message("m", "model path");
        exit(1);
    }
    if (source_text_path == NULL) {
        print_required_option_message("d", "source text path");
        exit(1);
    }
    if (embeddings_path == NULL) {
        print_required_option_message("e", "embeddings path");
        exit(1);
    }

    /* Map source text */
    char *full_source_text;
    size_t full_text_length;
    if (map_text(source_text_path, &full_source_text, &full_text_length) == -1) {
        LOG_ERROR("Failed to map text");
        exit(1);
    }
#define CLEANUP() \
    do { \
        munmap(full_source_text); \
    } while (false)

    /* Split source text into chunks */
    struct chunks chunks;
    {
        struct chunker chunker;
        if (init_char_chunker(&chunker, '\0') == -1) {
            LOG_ERROR("Failed to initialize chunker");
            exit(1);
        }

        if (get_chunks(&chunker, full_source_text, full_text_length, &chunks) == -1) {
            LOG_ERROR("Failed to split text into chunks");
            CLEANUP();
            dispose_chunker(&chunker);
            exit(1);
        }
        dispose_chunker(&chunker);
    }

    struct llama_model *model;
    {
        struct llama_model_params params = llama_model_default_params();
        model = llama_model_load_from_file(model_path, params);
    }
#undef CLEANUP
#define CLEANUP() \
    do { \
        dispose_chunks(chunks); \
        munmap(full_source_text); \
        llama_model_free(model); \
    } while (false)

    /* Initialize embedder */ 
    struct embedder embedder;
    {
        struct llama_embedder_params params = {
            .n_tokenizer_threads = n_tokenizer_threads,
            .context_size = context_size,
            .batch_size = batch_size,
            .ubatch_size = ubatch_size,
            .max_sequences_per_batch = max_sequences_per_batch
        };
        struct embedder llama_embedder;
        if (init_llama_embedder(&llama_embedder, model, &params) == -1) {
            LOG_ERROR("Failed to initialize embedder");
            CLEANUP();
            exit(1);
        }
        if (init_postprocessing_embedder(
                    &embedder, &llama_embedder, &embedding_postprocessing) == -1) {
            LOG_ERROR("Failed to wrap llama embedder with postprocessing");
            dispose_embedder(llama_embedder);
            CLEANUP();
            exit(1);
        }
    }
#undef CLEANUP
#define CLEANUP() \
    do { \
        dispose_chunks(chunks); \
        munmap(full_source_text); \
        llama_model_free(model); \
        dispose_embedder(embedder); \
    } while (false)

    /* Prepare and cache embeddings */
    struct embeddings embeddings;
    if (load_embeddings(embeddings_path, &chunks, &embedder, &embeddings) == -1) {
        LOG_ERROR("Failed to load or compute embeddings");
        CLEANUP();
        exit(1);
    }
#undef CLEANUP
#define CLEANUP() \
    do { \
        dispose_chunks(chunks); \
        munmap(full_source_text); \
        dispose_embedder(embedder); \
        llama_model_free(model); \
        dispose_embeddings(embeddings); \
    } while (false)
    
    if (sizeof(double) > SIZE_MAX / chunks->count) {
        LOG_ERROR("Number overflow");
        CLEANUP();
        exit(1);
    }
    double *similarity_scores = malloc(sizeof(double) * chunk->count);
    if (similarity_scores == NULL) {
        LOG_ERROR("Allocation failed");
        CLEANUP();
        exit(1);
    }
    double *sorted_text_indices = malloc(sizeof(size_t) * chunk->count);
    if (sorted_text_indices == NULL) {
        LOG_ERROR("Allocation failed");
        CLEANUP();
        exit(1);
    }

    /* Stdio loop */
    const int buffer_size = (1 << 24);
    char *read_buffer = malloc(sizeof(char) * buffer_size);
    while (true) {
        char *query = NULL;
        ssize_t top_k = -1;
        if (fgets(read_buffer, buffer_size, stdin) == NULL) {
            LOG_ERROR("Failed to read input");
            CLEANUP();
            exit(1);
        }
        cJSON *request = cJSON_Parse(read_buffer);
        if (request == NULL) {
            const char *error_ptr = cJSON_GetErrorPtr();
            LOG_ERROR("Failed to parse JSON: %s", error_ptr);
            continue;
        }
        if (!cJSON_IsObject(request)) {
            LOG_ERROR("Unexpected JSON structure. Root should be object");
            cJSON_Delete(request);
            continue;
        }
        cJSON *child = request->child;
        if (child == NULL) {
            LOG_ERROR("Request body contains no keys");
            cJSON_Delete(request);
            continue;
        }
        bool error_occurred = false;
        while (true) {
            if (child->string == "top_k") {
                if (!cJSON_IsNumber(child)) {
                    LOG_ERROR("Request \"top_k\" key should hold a number");
                    cJSON_Delete(request);
                    error_occurred = true;
                    break;
                }
                if (child->valuedouble < 1) {
                    LOG_ERROR("Request \"top_k\" key must be at least 1");
                    cJSON_Delete(request);
                    error_occurred = true;
                    break;
                }
                if (child->valuedouble != trunc(child->valuedouble)) {
                    LOG_ERROR("Request \"top_k\" key must be an integer");
                    cJSON_Delete(request);
                    error_occurred = true;
                    break;
                }
                top_k = (ssize_t)child->valuedouble;
            } else if (child->string == "query") {
                if (!cjSON_IsString(child)) {
                    LOG_ERROR("Request \"query\" key must be a string");
                    cJSON_Delete(request);
                    error_occurred = true;
                    break;
                }
                query = child->valuestring;
            } else {
                LOG_ERROR(
                        "Request must be JSON object with two keys:"
                        " \"top_k\" and \"query\"."
                        " No other keys are allowed");
                cJSON_Delete(request);
                error_occurred = true;
                break;
            }
            if (child.next == NULL) break;
        }
        if (error_occurred) {
            cJSON_Delete(request);
            continue;
        }
        if (top_k == -1) {
            LOG_ERROR("Expected \"top_k\" in request");
            cJSON_Delete(request);
            continue;
        }
        if (query == NULL) {
            LOG_ERROR("Expected \"query\" in request");
            cJSON_Delete(request);
            continue;
        }

        size_t query_length = strlen(query);

        struct embeddings query_embedding;
        {
            if (get_embeddings(
                    embedder,
                    &query,
                    &query_length,
                    1,
                    query_embedding) == -1) {
                CLEANUP();
                cJSON_Delete(request);
                exit(1);
            }
        }
        {
            size_t vector_offset = 0;
            for (size_t i = 0; i < chunks->count; i++) {
                similarity_scores[i] = dot_product(
                        embeddings->embeddings + vector_offset, 
                        query_embedding->embeddings,
                        embeddings->dimension_count);
                vector_offset += embeddings->dimension_count;
            }
            qsort(
                    sorted_text_indices, 
                    chunks->count, 
                    sizeof(size_t),
                    &compare_similarity,
                    similarity_scores);
        }
        cJSON_Delete(request);
        cJSON *response = cJSON_CreateArray();
        if (response == NULL) {
            LOG_ERROR("Response JSON array creation failed: %s", cJSON_GetError());
            CLEANUP();
            exit(1);
        }
        for (size_t i = 0; i < top_k && i < chunks->count; i++) {
            char *text_string = calloc(sizeof(char), (chunks->sizes[i] + 1));
            if (text_string == NULL) {
                LOG_ERROR("Allocation failed");
                CLEANUP();
                cJSON_Delete(request);
                exit(1);
            }
            memcpy(text_string, chunks->texts[i], chunks->sizes[i]);

            cJSON *response_item = cJSON_CreateString(text_string);
            free(text_string);

            cJSON_AddItemToArray(response, response_item);
        }
        char *response_string = cJSON_Print(response);
        cJSON_Delete(response);
        if (puts(response_string) == -1) {
            LOG_ERROR("puts failed");
            CLEANUP();
            free(response_string);
            exit(1);
        }
        free(response_string);
    }
}


            

                



        




    }

    /* Cleanup */
    CLEANUP();
    exit(EXIT_SUCCESS);
}
