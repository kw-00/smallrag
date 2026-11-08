#include "smallrag/embedder.h"

#if BUILD_LLAMA
#include "smallrag/llama_embedder.h"
#ifndef GGUF_MODEL_PATHS
#error "GGUF_MODEL_PATHS expected"
#endif
#endif

#if BUILD_ONNX
#include "smallrag/onnx_embedder.h"
#ifndef ONNX_MODEL_PATH
#error "ONNX_MODEL_PATH expected"
#endif
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void test_embedder(struct embedder *embedder)
{
    /* The first text deliberately has no terminating NUL. */
    char first[] = {'H', 'i', '!', 'X'};
    char *texts[] = {first, "A fox runs through the forest.", "Café by the sea."};
    size_t lengths[] = {3, strlen(texts[1]), strlen(texts[2])};
    struct embeddings batch;
    assert(get_embeddings(embedder, texts, lengths, 3, &batch) == 0);
    assert(batch.embeddings != NULL && batch.dispose != NULL);
    assert(batch.embedding_count == 3 && batch.dimension_count > 0);
    for (size_t i = 0; i < batch.embedding_count * batch.dimension_count; i++) {
        assert(isfinite(batch.embeddings[i]));
    }
    assert(texts[0] == first && memcmp(first, "Hi!X", sizeof(first)) == 0);
    assert(lengths[0] == 3 && lengths[1] == strlen(texts[1]) && lengths[2] == strlen(texts[2]));

    size_t dimension_count = batch.dimension_count;
    batch.dispose(&batch);
    /* Compare explicit lengths at the same batch shape, avoiding backend-specific
     * floating-point differences between single and mixed batches. */
    assert(get_embeddings(embedder, texts, lengths, 1, &batch) == 0);
    assert(batch.embedding_count == 1 && batch.dimension_count == dimension_count);
    char *single_text[] = {"Hi!"};
    size_t single_length[] = {3};
    struct embeddings single;
    assert(get_embeddings(embedder, single_text, single_length, 1, &single) == 0);
    assert(single.embedding_count == 1 && single.dimension_count == batch.dimension_count);
    for (size_t i = 0; i < single.dimension_count; i++) {
        assert(isfinite(single.embeddings[i]));
        assert(fabsf(single.embeddings[i] - batch.embeddings[i]) < 0.001f);
    }
    single.dispose(&single);
    batch.dispose(&batch);

    struct embeddings invalid;
    assert(get_embeddings(embedder, texts, lengths, 0, &invalid) == -1);
    size_t empty_length[] = {0};
    assert(get_embeddings(embedder, single_text, empty_length, 1, &invalid) == -1);
    char long_text[2048];
    for (size_t i = 0; i < sizeof(long_text); i += 4) {
        memcpy(long_text + i, "Hi. ", 4);
    }
    char *long_texts[] = {long_text};
    size_t long_lengths[] = {sizeof(long_text)};
    assert(get_embeddings(embedder, long_texts, long_lengths, 1, &invalid) == -1);
    /* A failed request must leave the embedder usable. */
    assert(get_embeddings(embedder, single_text, single_length, 1, &single) == 0);
    single.dispose(&single);
}

int main(void)
{
#if BUILD_LLAMA
    char model_paths[] = GGUF_MODEL_PATHS;
    char *saveptr = NULL;
    char *path = strtok_r(model_paths, ",", &saveptr);
    while (path != NULL) {
        struct llama_model_params model_params = llama_model_default_params();
        struct llama_model *model = llama_model_load_from_file(path, model_params);
        assert(model != NULL);
        struct llama_embedder_params params = {
            .n_tokenizer_threads = 2,
            .batch_size = 64,
            .max_sequences_per_batch = 2
        };
        struct embedder embedder;
        assert(init_llama_embedder(&embedder, model, &params) == 0);
        test_embedder(&embedder);
        dispose_embedder(&embedder);
        llama_model_free(model);
        path = strtok_r(NULL, ",", &saveptr);
    }
#endif
#if BUILD_ONNX
    struct onnx_embedder_params params = {
        .n_tokenizer_threads = 2,
        .batch_size = 64,
        .max_sequences_per_batch = 2
    };
    struct embedder embedder;
    assert(init_onnx_embedder(&embedder, ONNX_MODEL_PATH, &params) == 0);
    struct onnx_embedder_params invalid_params = params;
    invalid_params.batch_size = 0;
    assert(init_onnx_embedder(&embedder, ONNX_MODEL_PATH, &invalid_params) == -1);
    test_embedder(&embedder);
    dispose_embedder(&embedder);
#endif
    return 0;
}
