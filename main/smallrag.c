#include "smallrag/data_loading.h"
#include "smallrag/normalization.h"
#include "smallrag/dot_product.h"
#include "smallrag/embedder.h"
#include "smallrag/embeddings.h"
#include "smallrag/llama_embedder.h"
#include "smallrag/postprocessing_embedder.h"
#include "smallrag/chunker.h"
#include "smallrag/chunks.h"
#include "smallrag/char_chunker.h"
#include "smallrag/prefix_char_chunker.h"
#include "smallrag/allocation.h"
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
"   [ -P <chunk-prefix> ] \\\n"
"   [ -t <n-tokenizer-threads> ] \\\n"
"   [ -b <batch-size> ] \\\n"
"   [ -s <max-seq> ]\n"
"\n\n"
"Options:\n"
"-h - print this help and exit\n"
"- model-path - path to .gguf embedding model\n"
"- source-text - comma-separated paths to source text files. Each file must exist and contain text\n"
" fragments delimited using NUL bytes.\n"
"- embeddings-path - comma-separated paths to files that contain/will contain text embeddings.\n"
" Both lists must contain the same number of paths, with corresponding files in the same positions.\n"
" If an embedding file doesn't exist or is empty, embeddings are created for its source text and\n"
" saved there. Otherwise, its embeddings are used for retrieval of text fragments.\n"
" Do not attempt to create the embedding files yourself - embeddings are managed by the application.\n"
"- query-prefix - text prepended directly to each query before embedding. If left unspecified, no"
" prefix is added.\n"
"- chunk-prefix - text prepended directly to each text chunk before embedding. If left unspecified, no"
" prefix is added. It is not included in responses.\n"
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
" Splitting the query JSON into multiple lines is an error.\n"
"\n\n"
"The output format is as follows:\n"
"[\"No known bear species live in Antarctica.\", \"The grizzly bear is endemic to North America.\", ...]\n"
"\n\n"
"Output results are ranked from most relevant to least relevant. For each query, a single line of output"
" containing a JSON list like this is sent via standard ouput.\n";

/* -h — print help and exit
 * -m — model path
 * -d — comma-separated source text paths
 * -e — comma-separated embedding paths
 * -p — query prefix
 * -P — chunk prefix
 * -t — number of tokenizer threads
 * -b — batch size
 * -s — max sequences per batch
 */
static const char *optstring = "hm:d:e:p:P:t:b:s:";

struct similarity {
    size_t text_index;
    double score;
};

struct source_data {
    char *text;
    size_t text_length;
    bool text_mapped;
    struct chunks chunks;
    bool chunks_initialized;
    struct embeddings embeddings;
    bool embeddings_initialized;
};

static int parse_filenames(const char *value, char ***filenames, size_t *filename_count)
{
    size_t length = strlen(value);
    if (length == 0) {
        LOG_ERROR("Filename list contains an empty filename");
        return -1;
    }

    size_t count = 1;
    for (size_t i = 0; i < length; i++) {
        if (value[i] == ',') {
            if (i == 0 || i == length - 1 || value[i - 1] == ',') {
                LOG_ERROR("Filename list contains an empty filename");
                return -1;
            }
            count++;
        }
    }

    size_t storage_size;
    if (__builtin_add_overflow(length, (size_t)1, &storage_size)) {
        LOG_ERROR("Number overflow");
        return -1;
    }
    char **paths;
    char *storage;
    if (allocate_spans(
                2,
                &paths,
                sizeof(char *),
                count,
                &storage,
                sizeof(char),
                storage_size,
                _Alignof(char)) == -1) {
        LOG_ERROR("Allocation failed");
        return -1;
    }

    memcpy(storage, value, storage_size);
    paths[0] = storage;
    size_t path_index = 1;
    for (size_t i = 0; i < length; i++) {
        if (storage[i] == ',') {
            storage[i] = '\0';
            paths[path_index++] = storage + i + 1;
        }
    }
    *filenames = paths;
    *filename_count = count;
    return 0;
}

static void dispose_sources(struct source_data *sources, size_t source_count)
{
    if (sources == NULL) {
        return;
    }
    for (size_t i = 0; i < source_count; i++) {
        struct source_data *source = &sources[i];
        if (source->embeddings_initialized && source->embeddings.dispose != NULL) {
            source->embeddings.dispose(&source->embeddings);
        }
        if (source->chunks_initialized && source->chunks.dispose != NULL) {
            dispose_chunks(&source->chunks);
        }
        if (source->text_mapped) {
            if (munmap(source->text, source->text_length) == -1) {
                perror("munmap");
                LOG_ERROR("Failed to unmap source text");
            }
        }
    }
    free(sources);
}

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
    char *source_text_paths_arg = NULL;
    char *embeddings_paths_arg = NULL;
    const char *query_prefix = "";
    const char *chunk_prefix = NULL;
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
                source_text_paths_arg = optarg;
                break;
            case 'e':
                embeddings_paths_arg = optarg;
                break;
            case 'p':
                query_prefix = optarg;
                break;
            case 'P':
                chunk_prefix = optarg;
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
    if (source_text_paths_arg == NULL) {
        print_required_option_message("d", "source text paths");
        return EXIT_FAILURE;
    }
    if (embeddings_paths_arg == NULL) {
        print_required_option_message("e", "embedding paths");
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

    char **source_text_paths = NULL;
    char **embedding_paths = NULL;
    size_t source_count = 0;
    size_t embedding_path_count = 0;
    struct source_data *sources = NULL;
    struct chunks **source_chunks;
    struct embeddings **source_embeddings;
    struct chunker chunker;
    bool chunker_initialized = false;
    struct chunks chunks;
    bool chunks_initialized = false;
    struct llama_model *model = NULL;
    struct embedder llama_embedder;
    bool llama_embedder_initialized = false;
    struct embedder embedder;
    bool embedder_initialized = false;
    struct embeddings *embeddings = NULL;
    struct similarity *similarities = NULL;
    char *read_buffer = NULL;
    size_t read_buffer_capacity = 0;

#define CLEANUP() \
    do { \
        free(read_buffer); \
        free(similarities); \
        if (embeddings != NULL && embeddings->dispose != NULL) { \
            embeddings->dispose(embeddings); \
            embeddings = NULL; \
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
        if (chunker_initialized) { \
            dispose_chunker(&chunker); \
            chunker_initialized = false; \
        } \
        dispose_sources(sources, source_count); \
        sources = NULL; \
        free(source_text_paths); \
        free(embedding_paths); \
    } while (false)

    /* Parse filenames */
    if (parse_filenames(source_text_paths_arg, &source_text_paths, &source_count) == -1
            || parse_filenames(embeddings_paths_arg, &embedding_paths, &embedding_path_count) == -1) {
        LOG_ERROR("Failed to parse filenames");
        CLEANUP();
        return EXIT_FAILURE;
    }
    if (source_count != embedding_path_count) {
        LOG_ERROR("Source text and embedding filename counts must match");
        CLEANUP();
        return EXIT_FAILURE;
    }
    if (allocate_spans(
                3,
                &sources,
                sizeof(*sources),
                source_count,
                &source_chunks,
                sizeof(struct chunks *),
                source_count,
                _Alignof(struct chunks *),
                &source_embeddings,
                sizeof(struct embeddings *),
                source_count,
                _Alignof(struct embeddings *)) == -1) {
        LOG_ERROR("Allocation failed");
        CLEANUP();
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < source_count; i++) {
        sources[i] = (struct source_data) {0};
        source_chunks[i] = &sources[i].chunks;
        source_embeddings[i] = &sources[i].embeddings;
    }

    /* Initialize chunker */
    {
        int status;
        if (chunk_prefix == NULL) {
            status = init_char_chunker(&chunker, '\0');
        } else {
            status = init_prefix_char_chunker(&chunker, '\0', chunk_prefix);
        }
        if (status == -1) {
            LOG_ERROR("Failed to initialize chunker");
            CLEANUP();
            return EXIT_FAILURE;
        }
        chunker_initialized = true;
    }

    /* Map and split source texts */
    for (size_t i = 0; i < source_count; i++) {
        if (map_text(source_text_paths[i], &sources[i].text, &sources[i].text_length) == -1) {
            LOG_ERROR("Failed to map text: %s", source_text_paths[i]);
            CLEANUP();
            return EXIT_FAILURE;
        }
        sources[i].text_mapped = true;
        if (get_chunks(
                    &chunker, sources[i].text, sources[i].text_length, source_chunks[i]) == -1) {
            LOG_ERROR("Failed to split text into chunks: %s", source_text_paths[i]);
            CLEANUP();
            return EXIT_FAILURE;
        }
        sources[i].chunks_initialized = true;
        if (source_chunks[i]->count == 0) {
            LOG_ERROR("Source text contains no non-empty chunks: %s", source_text_paths[i]);
            CLEANUP();
            return EXIT_FAILURE;
        }
        if (source_chunks[i]->count > INT32_MAX) {
            LOG_ERROR("Source text contains too many chunks for the embedder: %s", source_text_paths[i]);
            CLEANUP();
            return EXIT_FAILURE;
        }
    }
    dispose_chunker(&chunker);
    chunker_initialized = false;

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
    for (size_t i = 0; i < source_count; i++) {
        if (load_embeddings(
                    embedding_paths[i], source_chunks[i], &embedder, source_embeddings[i]) == -1) {
            LOG_ERROR("Failed to load or compute embeddings: %s", embedding_paths[i]);
            CLEANUP();
            return EXIT_FAILURE;
        }
        sources[i].embeddings_initialized = true;
        struct embeddings *source_embedding = source_embeddings[i];
        if (source_embedding->embeddings == NULL || source_embedding->dispose == NULL
                || source_embedding->embedding_count != source_chunks[i]->count
                || source_embedding->dimension_count == 0) {
            LOG_ERROR("Loaded embeddings do not match source chunks: %s", embedding_paths[i]);
            CLEANUP();
            return EXIT_FAILURE;
        }
    }

    /* Concatenate source data */
    if (concat_chunks(source_chunks, source_count, &chunks) == 1) {
        LOG_ERROR("Failed to concatenate chunks");
        CLEANUP();
        return EXIT_FAILURE;
    }
    chunks_initialized = true;
    if (concat_embeddings(source_embeddings, source_count, &embeddings) == 1) {
        LOG_ERROR("Failed to concatenate embeddings");
        CLEANUP();
        return EXIT_FAILURE;
    }
    dispose_sources(sources, source_count);
    sources = NULL;

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
    size_t chunk_prefix_length = chunk_prefix == NULL ? 0 : strlen(chunk_prefix);

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
                || query_embedding.dimension_count != embeddings->dimension_count) {
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
                        embeddings->embeddings + vector_offset,
                        query_embedding.embeddings,
                        embeddings->dimension_count);
                if (!isfinite(score)) {
                    LOG_ERROR("Similarity calculation produced a non-finite score");
                    query_embedding.dispose(&query_embedding);
                    CLEANUP();
                    return EXIT_FAILURE;
                }
                similarities[i].text_index = i;
                similarities[i].score = score;
                vector_offset += embeddings->dimension_count;
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

            cJSON *response_item;
            if (chunk_prefix == NULL) {
                response_item = cJSON_CreateString(text_string);
            } else {
                response_item = cJSON_CreateString(text_string + chunk_prefix_length);
            }
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
