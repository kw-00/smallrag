#include "smallrag/llama_embedder.h"
#include "smallrag/embedder.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static max_align_t model_storage;
static max_align_t context_storage;
static struct llama_context_params received_params;
static size_t context_init_count;
static size_t context_free_count;
static bool fail_context_init;

static int sentinel_get_embeddings(
        struct embedder *embedder,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings)
{
    (void)embedder;
    (void)texts;
    (void)text_lengths;
    (void)text_count;
    (void)embeddings;
    return -1;
}

static void sentinel_dispose(struct embedder *embedder)
{
    (void)embedder;
}

static struct embedder sentinel_embedder(void)
{
    return (struct embedder) {
        .get_embeddings = &sentinel_get_embeddings,
        .dispose = &sentinel_dispose,
        .data = &model_storage
    };
}

static void assert_sentinel_embedder(const struct embedder *embedder)
{
    assert(embedder->get_embeddings == &sentinel_get_embeddings);
    assert(embedder->dispose == &sentinel_dispose);
    assert(embedder->data == &model_storage);
}

struct llama_context *llama_init_from_model(
        struct llama_model *model, struct llama_context_params params)
{
    assert(model == (struct llama_model *)&model_storage);
    received_params = params;
    context_init_count++;
    return fail_context_init ? NULL : (struct llama_context *)&context_storage;
}

void llama_free(struct llama_context *context)
{
    assert(context == (struct llama_context *)&context_storage);
    context_free_count++;
}

static void expect_invalid_params(struct llama_embedder_params params)
{
    struct embedder embedder = sentinel_embedder();
    size_t previous_init_count = context_init_count;
    assert(init_llama_embedder(
                &embedder, (struct llama_model *)&model_storage, &params) == -1);
    assert(context_init_count == previous_init_count);
    assert_sentinel_embedder(&embedder);
}

int main(void)
{
    const size_t batch_sizes[] = {1, 128, 2048, INT32_MAX};
    struct llama_context_params defaults = llama_context_default_params();
    size_t max_sequences_per_batch = defaults.n_seq_max == 8 ? 9 : 8;
    struct llama_embedder_params params = {
        .n_tokenizer_threads = 2,
        .batch_size = 1,
        .max_sequences_per_batch = max_sequences_per_batch
    };

    for (size_t i = 0; i < sizeof(batch_sizes) / sizeof(batch_sizes[0]); i++) {
        params.batch_size = batch_sizes[i];
        struct embedder embedder;
        assert(init_llama_embedder(
                    &embedder, (struct llama_model *)&model_storage, &params) == 0);
        assert(received_params.n_ctx == params.batch_size);
        assert(received_params.n_batch == params.batch_size);
        assert(received_params.n_ubatch == params.batch_size);
        assert(received_params.n_seq_max == params.max_sequences_per_batch);
        assert(received_params.embeddings);
        dispose_embedder(&embedder);
        assert(context_free_count == i + 1);
    }

    params.batch_size = 0;
    expect_invalid_params(params);
    params.batch_size = (size_t)INT32_MAX + 1;
    expect_invalid_params(params);
    params.batch_size = 128;
    params.max_sequences_per_batch = 0;
    expect_invalid_params(params);
#if SIZE_MAX > UINT32_MAX
    params.max_sequences_per_batch = (size_t)UINT32_MAX + 1;
    expect_invalid_params(params);
#endif

    params.max_sequences_per_batch = max_sequences_per_batch;
    fail_context_init = true;
    struct embedder embedder = sentinel_embedder();
    size_t previous_init_count = context_init_count;
    assert(init_llama_embedder(
                &embedder, (struct llama_model *)&model_storage, &params) == -1);
    assert(context_init_count == previous_init_count + 1);
    assert_sentinel_embedder(&embedder);
    assert(context_free_count == sizeof(batch_sizes) / sizeof(batch_sizes[0]));
    return 0;
}
