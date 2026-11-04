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

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mman.h>

static const char *usage =
"Stdio-based information retrieval server.\n"
"Usage:\n"
"smallrag -h\n"
"smallrag \\\n"
"   -m <model-path> \\\n"
"   -d <source-text> \\\n"
"   -e <embeddings-path> \\\n"
"   [ -p <query-prefix> ] \\\n"
"   [ -t <n-tokenizer-threads> ] \\\n"
"   [ -b <batch-size> ] \\\n"
"   [ -s <max-seq> ]\n"
"\n\n"
"Options:\n"
"-h - print this help and exit\n"
"- model-path - path to .gguf embedding model\n"
"- source-text - path to source text. Must exist as a file. Source text should contain text fragments\n"
"   delimited using NUL bytes,\n"
"- embeddings-path - path to a file that contains/will contain text embeddings. If the file doesn't exist,"
" it embeddings are created for the source text and saved under the path passed via -e.\n"
" if the file does exist, then it is used for retrieval of text fragments.\n"
" Do not attempt to create it yourself - embeddings are managed by the application."
" Pass a whatever path you want the application to use for storing embeddings.\n"
"- query-prefix - text prepended directly to each query before embedding (default: empty)."
"\n\n"
"The remaining arguments are used to configure the llama.cpp backend. They are all optional.\n"
"- n-tokenizer-threads - number of threads to use for tokenization. Default will use as many threads"
" as there are logical processors.\n"
"- batch-size - maximum total number of tokens in each embedding batch, including special tokens."
" Each text must fit within this limit. Default: 2048. Larger values can increase memory use.\n"
"- <max-seq> - max number of sequences per logical batch.\n"
"\n\n"
"To use smallrag, make sure to enter input into the smallrag process's standard input. The input"
" format is as follows:\n"
"{\"query\": \"How many bear species exist in Antarctica?\", \"top_k\": 10}"
"\n\n"
"Generally, the input has the form of JSON lines. One line = one query, according to the format above."
" Splitting the query JSON into multiple lines is an error."
"\n\n"
"The output format is as follows:\n"
"[\"No known bear species live in Antarctica.\", \"The grizzly bear is endemic to North America.\", ...]\n"
"\n\n"
"Output results are ranked from most relevant to least relevant. For each query, a single line of output"
" containing a JSON list like this is sent via standard ouput.";

/* -h — print help and exit
 * -m — model path
 * -d — source text path
 * -e — embeddings path
 * -p — query prefix
 * -t — number of tokenizer threads
 * -b — batch size
 * -s — max sequences per batch
 */
static const char *optstring = "hm:d:e:p:t:b:s:";

struct similarity {
    size_t text_index;
    double score;
};

static size_t parse_size_t_arg(const char *option_short)
{
    char *end;
    errno = 0;
    uintmax_t value = strtoumax(optarg, &end, 10);
    if (optarg[0] == '\0' || optarg[0] == '-' || end == optarg || *end != '\0'
            || errno == ERANGE || value > SIZE_MAX) {
        LOG_ERROR("Invalid value for option -%s: %s", option_short, optarg);
        exit(EXIT_FAILURE);
    }
    return (size_t)value;
}

static void print_required_option_message(const char *option_short, const char *option_long)
{
    fprintf(stderr, "Option -%s (%s) is required\n", option_short, option_long);
}

static int embedding_postprocessing(struct embeddings *embeddings)
{
    if (embeddings->embeddings == NULL || embeddings->embedding_count == 0
            || embeddings->dimension_count == 0
            || embeddings->embedding_count > SIZE_MAX / embeddings->dimension_count) {
        LOG_ERROR("Invalid embeddings for normalization");
        return -1;
    }

    size_t vector_offset = 0;
    for (size_t i = 0; i < embeddings->embedding_count; i++) {
        if (l2_normalize(
                    embeddings->embeddings + vector_offset,
                    embeddings->dimension_count) == -1) {
            LOG_ERROR("Normalization failed");
            return -1;
        }
        vector_offset += embeddings->dimension_count;
    }
    return 0;
}

static int compare_similarity(const void *left, const void *right)
{
    const struct similarity *similarity_x = left;
    const struct similarity *similarity_y = right;
    if (similarity_x->score > similarity_y->score) {
        return -1;
    }
    if (similarity_x->score < similarity_y->score) {
        return 1;
    }
    if (similarity_x->text_index < similarity_y->text_index) {
        return -1;
    }
    if (similarity_x->text_index > similarity_y->text_index) {
        return 1;
    }
    return 0;
}

static int parse_request(cJSON *request, char **query, double *top_k)
{
    *query = NULL;

    if (!cJSON_IsObject(request)) {
        LOG_ERROR("Unexpected JSON structure. Root should be an object");
        return -1;
    }

    cJSON *top_k_item = NULL;
    cJSON *query_item = NULL;
    size_t key_count = 0;
    for (cJSON *child = request->child; child != NULL; child = child->next) {
        key_count++;
        if (child->string == NULL) {
            LOG_ERROR("Request object contains a key without a name");
            return -1;
        }
        if (strcmp(child->string, "top_k") == 0) {
            if (top_k_item != NULL) {
                LOG_ERROR("Request contains more than one \"top_k\" key");
                return -1;
            }
            top_k_item = child;
        } else if (strcmp(child->string, "query") == 0) {
            if (query_item != NULL) {
                LOG_ERROR("Request contains more than one \"query\" key");
                return -1;
            }
            query_item = child;
        } else {
            LOG_ERROR(
                    "Request must contain only the \"top_k\" and \"query\" keys");
            return -1;
        }
    }

    if (key_count != 2) {
        LOG_ERROR("Request must contain exactly two keys: \"top_k\" and \"query\"");
        return -1;
    }
    if (top_k_item == NULL) {
        LOG_ERROR("Expected \"top_k\" in request");
        return -1;
    }
    if (query_item == NULL) {
        LOG_ERROR("Expected \"query\" in request");
        return -1;
    }
    if (!cJSON_IsNumber(top_k_item)) {
        LOG_ERROR("Request \"top_k\" key should hold a number");
        return -1;
    }
    if (!isfinite(top_k_item->valuedouble) || top_k_item->valuedouble < 1.0) {
        LOG_ERROR("Request \"top_k\" key must be a finite number of at least 1");
        return -1;
    }
    if (trunc(top_k_item->valuedouble) != top_k_item->valuedouble) {
        LOG_ERROR("Request \"top_k\" key must be an integer");
        return -1;
    }
    if (!cJSON_IsString(query_item) || cJSON_GetStringValue(query_item) == NULL) {
        LOG_ERROR("Request \"query\" key must be a string");
        return -1;
    }
    if (cJSON_GetStringValue(query_item)[0] == '\0') {
        LOG_ERROR("Request \"query\" key must not be empty");
        return -1;
    }

    *query = cJSON_GetStringValue(query_item);
    *top_k = top_k_item->valuedouble;
    return 0;
}

int main(int argc, char *argv[])
{
    char *model_path = NULL;
    char *source_text_path = NULL;
    char *embeddings_path = NULL;
    const char *query_prefix = "";
    size_t n_tokenizer_threads = 0;
    size_t batch_size = 2048;
    size_t max_sequences_per_batch = 256;

    while (true) {
        int option = getopt(argc, argv, optstring);
        if (option == -1) {
            break;
        }
        switch (option) {
            case 'h':
                if (fputs(usage, stdout) == EOF || fflush(stdout) == EOF) {
                    perror("stdout");
                    return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
            case 'm':
                model_path = optarg;
                break;
            case 'd':
                source_text_path = optarg;
                break;
            case 'e':
                embeddings_path = optarg;
                break;
            case 'p':
                query_prefix = optarg;
                break;
            case 't':
                n_tokenizer_threads = parse_size_t_arg("t");
                break;
            case 'b':
                batch_size = parse_size_t_arg("b");
                break;
            case 's':
                max_sequences_per_batch = parse_size_t_arg("s");
                break;
            default:
                LOG_ERROR("Invalid command-line option");
                return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        LOG_ERROR("Unexpected positional argument: %s", argv[optind]);
        return EXIT_FAILURE;
    }
    if (model_path == NULL) {
        print_required_option_message("m", "model path");
        return EXIT_FAILURE;
    }
    if (source_text_path == NULL) {
        print_required_option_message("d", "source text path");
        return EXIT_FAILURE;
    }
    if (embeddings_path == NULL) {
        print_required_option_message("e", "embeddings path");
        return EXIT_FAILURE;
    }
    if (batch_size == 0 || max_sequences_per_batch == 0) {
        LOG_ERROR("Batch and max sequence sizes must be positive");
        return EXIT_FAILURE;
    }
    if (batch_size > INT32_MAX || max_sequences_per_batch > UINT32_MAX) {
        LOG_ERROR("A batch or max sequence size is too large");
        return EXIT_FAILURE;
    }

    char *full_source_text = NULL;
    size_t full_text_length = 0;
    bool source_text_mapped = false;
    struct chunks chunks;
    bool chunks_initialized = false;
    struct llama_model *model = NULL;
    struct embedder llama_embedder;
    bool llama_embedder_initialized = false;
    struct embedder embedder;
    bool embedder_initialized = false;
    struct embeddings embeddings;
    bool embeddings_initialized = false;
    struct similarity *similarities = NULL;
    char *read_buffer = NULL;
    size_t read_buffer_capacity = 0;

#define CLEANUP() \
    do { \
        free(read_buffer); \
        free(similarities); \
        if (embeddings_initialized && embeddings.dispose != NULL) { \
            embeddings.dispose(&embeddings); \
            embeddings_initialized = false; \
        } \
        if (embedder_initialized) { \
            dispose_embedder(&embedder); \
            embedder_initialized = false; \
        } \
        if (llama_embedder_initialized) { \
            dispose_embedder(&llama_embedder); \
            llama_embedder_initialized = false; \
        } \
        if (model != NULL) { \
            llama_model_free(model); \
            model = NULL; \
        } \
        if (chunks_initialized && chunks.dispose != NULL) { \
            dispose_chunks(&chunks); \
            chunks_initialized = false; \
        } \
        if (source_text_mapped) { \
            if (munmap(full_source_text, full_text_length) == -1) { \
                perror("munmap"); \
                LOG_ERROR("Failed to unmap source text"); \
            } \
            source_text_mapped = false; \
        } \
    } while (false)

    /* Map source text */
    if (map_text(source_text_path, &full_source_text, &full_text_length) == -1) {
        LOG_ERROR("Failed to map text");
        CLEANUP();
        return EXIT_FAILURE;
    }
    source_text_mapped = true;

    /* Split source text into chunks */
    {
        struct chunker chunker;
        if (init_char_chunker(&chunker, '\0') == -1) {
            LOG_ERROR("Failed to initialize chunker");
            CLEANUP();
            return EXIT_FAILURE;
        }

        if (get_chunks(&chunker, full_source_text, full_text_length, &chunks) == -1) {
            LOG_ERROR("Failed to split text into chunks");
            dispose_chunker(&chunker);
            CLEANUP();
            return EXIT_FAILURE;
        }
        chunks_initialized = true;
        dispose_chunker(&chunker);
    }
    if (chunks.count == 0) {
        LOG_ERROR("Source text contains no non-empty chunks");
        CLEANUP();
        return EXIT_FAILURE;
    }
    if (chunks.count > INT32_MAX) {
        LOG_ERROR("Source text contains too many chunks for the embedder");
        CLEANUP();
        return EXIT_FAILURE;
    }

    /* Load model */
    {
        struct llama_model_params params = llama_model_default_params();
        model = llama_model_load_from_file(model_path, params);
    }
    if (model == NULL) {
        LOG_ERROR("Failed to load model");
        CLEANUP();
        return EXIT_FAILURE;
    }

    /* Initialize embedder */
    {
        struct llama_embedder_params params = {
            .n_tokenizer_threads = n_tokenizer_threads,
            .batch_size = batch_size,
            .max_sequences_per_batch = max_sequences_per_batch
        };
        if (init_llama_embedder(&llama_embedder, model, &params) == -1) {
            LOG_ERROR("Failed to initialize embedder");
            CLEANUP();
            return EXIT_FAILURE;
        }
        llama_embedder_initialized = true;

        if (init_postprocessing_embedder(
                    &embedder, &llama_embedder, &embedding_postprocessing) == -1) {
            LOG_ERROR("Failed to wrap llama embedder with postprocessing");
            CLEANUP();
            return EXIT_FAILURE;
        }
        embedder_initialized = true;
        /* The wrapper now owns the llama embedder's resources. */
        llama_embedder_initialized = false;
    }

    /* Prepare and cache embeddings */
    if (load_embeddings(embeddings_path, &chunks, &embedder, &embeddings) == -1) {
        LOG_ERROR("Failed to load or compute embeddings");
        CLEANUP();
        return EXIT_FAILURE;
    }
    embeddings_initialized = true;
    if (embeddings.embeddings == NULL || embeddings.dispose == NULL
            || embeddings.embedding_count != chunks.count || embeddings.dimension_count == 0) {
        LOG_ERROR("Loaded embeddings do not match source chunks");
        CLEANUP();
        return EXIT_FAILURE;
    }

    size_t similarity_memory_size;
    if (__builtin_mul_overflow(
                chunks.count, sizeof(*similarities), &similarity_memory_size)) {
        LOG_ERROR("Number overflow");
        CLEANUP();
        return EXIT_FAILURE;
    }
    similarities = malloc(similarity_memory_size);
    if (similarities == NULL) {
        LOG_ERROR("Allocation failed");
        CLEANUP();
        return EXIT_FAILURE;
    }

    size_t query_prefix_length = strlen(query_prefix);

    /* Stdio loop */
    while (true) {
        ssize_t line_length = getline(&read_buffer, &read_buffer_capacity, stdin);
        if (line_length == -1) {
            if (feof(stdin)) {
                break;
            }
            perror("getline");
            LOG_ERROR("Failed to read input");
            CLEANUP();
            return EXIT_FAILURE;
        }

        cJSON *request = cJSON_ParseWithOpts(read_buffer, NULL, 1);
        if (request == NULL) {
            const char *error_ptr = cJSON_GetErrorPtr();
            LOG_ERROR(
                    "Failed to parse JSON: %s",
                    error_ptr == NULL ? "unknown error location" : error_ptr);
            continue;
        }

        char *query = NULL;
        double requested_top_k = 0.0;
        if (parse_request(request, &query, &requested_top_k) == -1) {
            cJSON_Delete(request);
            continue;
        }
        size_t query_length = strlen(query);
        char *prefixed_query = NULL;
        if (query_prefix_length != 0) {
            if (query_length > SIZE_MAX - query_prefix_length - 1) {
                LOG_ERROR("Query is too large");
                cJSON_Delete(request);
                CLEANUP();
                return EXIT_FAILURE;
            }
            prefixed_query = malloc(query_prefix_length + query_length + 1);
            if (prefixed_query == NULL) {
                LOG_ERROR("Allocation failed");
                cJSON_Delete(request);
                CLEANUP();
                return EXIT_FAILURE;
            }
            memcpy(prefixed_query, query_prefix, query_prefix_length);
            memcpy(prefixed_query + query_prefix_length, query, query_length + 1);
            query = prefixed_query;
            query_length += query_prefix_length;
        }

        struct embeddings query_embedding;
        {
            char *query_texts[] = {query};
            size_t query_lengths[] = {query_length};
            int result = get_embeddings(
                    &embedder,
                    query_texts,
                    query_lengths,
                    1,
                    &query_embedding);
            free(prefixed_query);
            if (result == -1) {
                LOG_ERROR("Failed to embed query");
                cJSON_Delete(request);
                CLEANUP();
                return EXIT_FAILURE;
            }
        }
        cJSON_Delete(request);

        if (query_embedding.embeddings == NULL || query_embedding.dispose == NULL
                || query_embedding.embedding_count != 1
                || query_embedding.dimension_count != embeddings.dimension_count) {
            LOG_ERROR("Query embedding does not match source embedding dimensions");
            if (query_embedding.dispose != NULL) {
                query_embedding.dispose(&query_embedding);
            }
            CLEANUP();
            return EXIT_FAILURE;
        }

        {
            size_t vector_offset = 0;
            for (size_t i = 0; i < chunks.count; i++) {
                double score = dot_product(
                        embeddings.embeddings + vector_offset,
                        query_embedding.embeddings,
                        embeddings.dimension_count);
                if (!isfinite(score)) {
                    LOG_ERROR("Similarity calculation produced a non-finite score");
                    query_embedding.dispose(&query_embedding);
                    CLEANUP();
                    return EXIT_FAILURE;
                }
                similarities[i].text_index = i;
                similarities[i].score = score;
                vector_offset += embeddings.dimension_count;
            }
            qsort(
                    similarities,
                    chunks.count,
                    sizeof(*similarities),
                    &compare_similarity);
        }
        query_embedding.dispose(&query_embedding);

        size_t response_count;
        if ((long double)requested_top_k >= (long double)chunks.count) {
            response_count = chunks.count;
        } else {
            response_count = (size_t)requested_top_k;
        }

        cJSON *response = cJSON_CreateArray();
        if (response == NULL) {
            LOG_ERROR("Response JSON array creation failed");
            CLEANUP();
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < response_count; i++) {
            size_t text_index = similarities[i].text_index;
            size_t text_length = chunks.sizes[text_index];
            if (text_length == SIZE_MAX) {
                LOG_ERROR("Text chunk is too large");
                cJSON_Delete(response);
                CLEANUP();
                return EXIT_FAILURE;
            }
            char *text_string = malloc(text_length + 1);
            if (text_string == NULL) {
                LOG_ERROR("Allocation failed");
                cJSON_Delete(response);
                CLEANUP();
                return EXIT_FAILURE;
            }
            memcpy(text_string, chunks.texts[text_index], text_length);
            text_string[text_length] = '\0';

            cJSON *response_item = cJSON_CreateString(text_string);
            free(text_string);
            if (response_item == NULL) {
                LOG_ERROR("Response JSON string creation failed");
                cJSON_Delete(response);
                CLEANUP();
                return EXIT_FAILURE;
            }
            if (!cJSON_AddItemToArray(response, response_item)) {
                LOG_ERROR("Failed to add response item");
                cJSON_Delete(response_item);
                cJSON_Delete(response);
                CLEANUP();
                return EXIT_FAILURE;
            }
        }

        char *response_string = cJSON_PrintUnformatted(response);
        cJSON_Delete(response);
        if (response_string == NULL) {
            LOG_ERROR("Response JSON serialization failed");
            CLEANUP();
            return EXIT_FAILURE;
        }
        if (fputs(response_string, stdout) == EOF
                || fputc('\n', stdout) == EOF
                || fflush(stdout) == EOF) {
            perror("stdout");
            LOG_ERROR("Failed to write response");
            free(response_string);
            CLEANUP();
            return EXIT_FAILURE;
        }
        free(response_string);
    }

    /* Cleanup */
    CLEANUP();
#undef CLEANUP
    return EXIT_SUCCESS;
}
