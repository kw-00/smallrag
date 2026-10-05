#include "smallrag/llama_embedder.h"

#include "smallrag/logging.h"
#include <sys/mman.h>
#include <stdint.h>

struct llama_embedder_data {
    struct llama_context *context;
    size_t n_threads;
}

static void dispose_embedder(struct embedder *embedder)
{
    free(embedder->data);
}

/* For each text from texts, tokens are produced.
 *
 * To get tokens for texts[i], use tokens[offset], where offset is the sum of all text_lenghts[j],
 * where j <  i.
 */
static int tokenize(
        struct llama_vocab *tokenizer,
        char **texts, 
        size_t *text_lengths, 
        size_t text_count, 
        llama_token **tokens, 
        size_t **token_counts)
{
    llama_token *token_mem;
    size_t *token_count_mem;
    // Allocating memory for tokens, assuming worst-case scenario where one byte of text
    // corresponds to one token
    {
        size_t full_text_size = 0;
        for (size_t i = 0; i < text_count; i++) {
            if (__builtin_add_overflow(full_text_size, text_lengths[i], &full_text_size)) {
                LOG_ERROR("Number overflow");
                return -1;
            }
        }

        size_t alloc_size = sizeof(llama_token) + sizeof(size_t);
        if (__builtin_mul_overflow(alloc_size, full_text_size, &alloc_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }

        token_mem = malloc(alloc_size);
        if (token_mem == NULL) {
            LOG_ERROR("Allocation failed");
            return -1;
        }
        token_count_mem = token_mem + full_text_size;
    }
    {
        size_t tokens_offset = 0;
        for (size_t i = 0; i < text_count; i++) {
            if (text_lengths[i] > INT32_MAX) {
                LOG_ERROR("Number overflow");
                return -1;
            }
            int32_t token_count = llama_tokenize(
                    tokenizer,
                    texts[i],
                    text_lengths[i],
                    token_mem + tokens_offset,
                    token_lengths[i],
                    true,
                    true);
            if (token_count < 0) {
                LOG_ERROR("Error during tokenization");
                return -1;
            }
            token_count_mem[i] = token_count;
        }
    }
    *tokens = token_mem;
    *token_counts = token_count_mem;
    return 0;
}

struct tokenize_task_context {
    struct llama_vocab *tokenizer;
    char **texts;
    size_t *text_lengths
    size_t text_count; 
    llama_token **tokens;
    size_t **token_counts;
}

static void *tokenize_task(void *args)
{
    static int success = 0;
    static int fail = -1;
    struct tokenize_task_context *context = (struct tokenize_task_context *)args;
    
    if (tokenize(
                context->tokenizer,
                context->texts,
                cntext->text_lengths,
                context->text_count,
                context->tokens,
                context->token_counts) == -1) {
        return &fail;
    }
    return &success;
}

static int get_embeddings(
        struct embedder *embedder,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings)
{
    // TODO - implement
}


int init_llama_embedder(struct embedder *embedder, struct llama_context *context, size_t n_threads)
{
    struct llama_embedder_data *data = malloc(sizeof(llama_embedder_data));
    if (data == NULL) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    data->context = context;
    data->n_threads = n_threads;
    embedder->data = data;
    embedder->dispose = &dispose_embedder;
}
