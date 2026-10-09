#include "smallrag/llama_embedder.h"

#include "smallrag/logging.h"
#include "smallrag/allocation.h"
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/param.h>

struct embedder_data {
    struct llama_context *context;
    size_t n_threads;
    size_t batch_size;
};

static void dispose_embedder(struct embedder *embedder)
{
    free(embedder->data);
}

/* For each text from texts, tokens are produced.
 *
 * To get tokens for texts[i], use tokens[offset], where offset is the sum of all text_lenghts[j],
 * where j <  i.
 */
struct tokenize_task_context {
    const struct llama_vocab *tokenizer;
    char **texts;
    size_t *text_lengths;
    size_t text_count; 
    llama_token *tokens;
    size_t *token_counts;
};

static void *tokenize_task(void *args)
{
    static int success = 0;
    static int fail = -1;
    struct tokenize_task_context *context = (struct tokenize_task_context *)args;

    {
        size_t token_offset = 0;
        for (size_t i = 0; i < context->text_count; i++) {
            if (context->text_lengths[i] > INT32_MAX) {
                LOG_ERROR("Number overflow");
                return &fail;
            }
            int32_t token_count = llama_tokenize(
                    context->tokenizer,
                    context->texts[i],
                    context->text_lengths[i],
                    context->tokens + token_offset,
                    context->text_lengths[i],
                    true,
                    true);
            if (token_count < 0) {
                LOG_ERROR("Error during tokenization");
                return &fail;
            }
            context->token_counts[i] = token_count;
            token_offset += context->text_lengths[i];
        }
    }
    return &success;
}

/* Returns tokens via tokens and token_counts out parameters and returns total token count or -1
 * on error.
 */
static ssize_t tokenize(
        const struct llama_vocab *tokenizer,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        size_t n_threads,
        llama_token **tokens,
        size_t **token_counts)
{
    size_t texts_per_thread = text_count / n_threads;
    size_t excess = text_count % n_threads;

    size_t n_workers;
    if (texts_per_thread == 0) {
        n_workers = excess;
    } else {
        n_workers = n_threads;
    }
    
    void *tokenization_memory; 
    pthread_t *thread_handles;
    struct tokenize_task_context *contexts;
    llama_token *token_storage;
    {
        /* Calculating memory for token_storage, assuming worst-case scenario where one byte of text
         * corresponds to one token */
        size_t full_text_size = 0;
        for (size_t i = 0; i < text_count; i++) {
            if (__builtin_add_overflow(full_text_size, text_lengths[i], &full_text_size)) {
                LOG_ERROR("Number overflow");
                return -1;
            }
        }
        if (allocate_spans(
                3,

                &thread_handles, 
                sizeof(pthread_t), 
                n_workers,

                &contexts, 
                sizeof(struct tokenize_task_context), 
                n_workers, 
                _Alignof(struct tokenize_task_context),

                &token_storage, 
                sizeof(llama_token), 
                full_text_size,
                _Alignof(llama_token)) == -1) {
            LOG_ERROR("Allocation failed");
            return -1;
        }
        // Memory handle for freeing
        tokenization_memory = thread_handles;

        #define ERROR_CLEANUP() free(tokenization_memory)
    }
    size_t *token_count_memory;
    {
        if (allocate_spans(1, &token_count_memory, sizeof(size_t), text_count) == -1) {
            LOG_ERROR("Allocation failed");
            ERROR_CLEANUP();
            return -1;
        }
#undef ERROR_CLEANUP
#define ERROR_CLEANUP() \
        do { \
            free(tokenization_memory); \
            free(token_count_memory); \
        } while (false)
    }
    
    {    
        size_t text_idx = 0;
        size_t tokens_offset = 0;
        for (int i = 0; i < n_workers; i++) {
            size_t texts_for_worker = texts_per_thread + (i < excess);
            struct tokenize_task_context *context = contexts + i;
            context->tokenizer = tokenizer;
            context->texts = texts + text_idx;
            context->text_lengths = text_lengths + text_idx;
            context->text_count = texts_for_worker;
            context->tokens = token_storage + tokens_offset;
            context->token_counts = token_count_memory + text_idx;
            if (pthread_create(
                        thread_handles + i,
                        NULL,
                        &tokenize_task,
                        context) != 0) {
                LOG_ERROR("Thread creation failed");
                ERROR_CLEANUP();
                return -1;
            }
            text_idx += texts_for_worker;
            for (
                    size_t relative_text_idx = 0; 
                    relative_text_idx < texts_for_worker; 
                    relative_text_idx++)
            {
                tokens_offset += context->text_lengths[relative_text_idx];
            }
        }
    }

    bool error_occurred = false;
    for (int i = 0; i < n_workers; i++) {
        int *worker_return;
        if (pthread_join(thread_handles[i], (void **)&worker_return) != 0) {
            LOG_ERROR("Joining worker ended in an error");
            error_occurred = true;
        } else if (*worker_return == -1) {
            LOG_ERROR("Worker thread ended with an error");
            error_occurred = true;
        }
    }
    if (error_occurred) {
        ERROR_CLEANUP();
        return -1;
    }
    
    llama_token *tokens_contiguous;
    ssize_t total_token_count = 0;
    {
        for (size_t i = 0; i < n_workers; i++) {
            struct tokenize_task_context *context = contexts + i; 
            for (
                    size_t relative_text_idx = 0; 
                    relative_text_idx < context->text_count; 
                    relative_text_idx++) {
                if (__builtin_add_overflow(
                            total_token_count, 
                            context->token_counts[relative_text_idx],
                            &total_token_count)) {
                    LOG_ERROR("Number overflow");
                    ERROR_CLEANUP();
                    return -1;
                }
            }
        }
        size_t alloc_size;
        if (__builtin_mul_overflow(sizeof(llama_token), total_token_count, &alloc_size)) {
            LOG_ERROR("Number overflow");
            ERROR_CLEANUP();
            return -1;
        }
        tokens_contiguous = malloc(alloc_size);
        if (tokens_contiguous == NULL) {
            LOG_ERROR("Allocation failed");
            ERROR_CLEANUP();
            return -1;
        }
#undef ERROR_CLEANUP
#define ERROR_CLEANUP() \
        do { \
            free(tokenization_memory); \
            free(token_count_memory); \
            free(tokens_contiguous) \
        } while (false)
    }
    {
        size_t destination_offset = 0;
        for (size_t i = 0; i < n_workers; i++) {
            struct tokenize_task_context *context = contexts + i;
            size_t source_offset = 0;
            for (size_t text_idx = 0; text_idx < context->text_count; text_idx++) {
                memcpy(
                        tokens_contiguous + destination_offset,
                        context->tokens + source_offset,
                        context->token_counts[text_idx] * sizeof(llama_token));
                destination_offset += context->token_counts[text_idx];
                source_offset += context->text_lengths[text_idx];
            }
        }
    }

    free(tokenization_memory);

    *tokens = tokens_contiguous;
    *token_counts = token_count_memory;
#undef ERROR_CLEANUP
    return total_token_count;
}


static int get_embeddings(
        struct embedder *embedder,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings)
{
    if (text_count == 0) {
        LOG_ERROR("text_count must be positive");
        return -1;
    }
    for (size_t i = 0; i < text_count; i++) {
        if (text_lengths[i] == 0) {
            LOG_ERROR("Each of text_lengths must be positive");
            return -1;
        }
    }
    struct embedder_data *embedder_data = (struct embedder_data *)embedder->data;
    struct llama_context *context = embedder_data->context;
    size_t n_threads = embedder_data->n_threads;
    size_t batch_size = embedder_data->batch_size;
    if (n_threads == 0) {
        n_threads = _SC_NPROCESSORS_ONLN;
    }
    const struct llama_model *model = llama_get_model(context);
    /* Check pooling type */
    {
        const enum llama_pooling_type pooling_type = llama_pooling_type(context);
        if (pooling_type == LLAMA_POOLING_TYPE_NONE) {
            LOG_ERROR("LLAMA_POOLING_TYPE_NONE not supported");
            return -1;
        }
        if (pooling_type == LLAMA_POOLING_TYPE_RANK) {
            LOG_ERROR("LLAMA_POOLING_TYPE_RANK not supported");
            return -1;
        }
    }

    llama_token *tokens;
    size_t *token_counts;
    ssize_t total_token_count;
    /* Tokenize */
    {
        const struct llama_vocab *vocab = llama_model_get_vocab(model);

        total_token_count = tokenize(
                vocab, texts, text_lengths, text_count, n_threads, &tokens, &token_counts);
        if (total_token_count == -1) {
            LOG_ERROR("Tokenization failed");
            return -1;
        }
#define ERROR_CLEANUP() \
        do { \
            free(tokens); \
            free(token_counts); \
        } while (false)
    }

    int32_t dimension_count = llama_model_n_embd_out(model);

    float *embedding_memory;
    /* Allocate memory for embeddings */
    {
        size_t total_vector_size;
        if (__builtin_mul_overflow(text_count, dimension_count, &total_vector_size)) {
            LOG_ERROR("Number overflow");
            ERROR_CLEANUP();
            return -1;
        }
        if (allocate_spans(1, &embedding_memory, sizeof(float), text_count) == -1) {
            LOG_ERROR("Allocation failed");
            ERROR_CLEANUP();
            return -1;
        }
#undef ERROR_CLEANUP
#define ERROR_CLEANUP() \
        do { \
            free(tokens); \
            free(token_counts); \
            free(embedding_memory); \
        } while (false)
    }


    /* Batch and decode */
    {
        if (batch_size > INT32_MAX) {
            LOG_ERROR("Number overflow");
            ERROR_CLEANUP();
            return -1;
        }
        struct llama_batch batch = llama_batch_init(batch_size, 0, text_count);
        batch.logits = NULL;
        batch.pos = NULL;
        for (size_t i = 0; i < batch_size; i++) {
            batch.n_seq_id[i] = 1;
        }

#undef ERROR_CLEANUP
#define ERROR_CLEANUP() \
        do { \
            free(tokens); \
            free(token_counts); \
            free(embedding_memory); \
            llama_batch_free(batch); \
        } while (false)

        const bool remainder_present = total_token_count % batch_size > 0;
        const size_t batch_count 
            = total_token_count / batch_size + remainder_present;

        size_t token_idx = 0;

        size_t sequence_idx = 0;
        size_t sequence_end = token_counts[sequence_idx];

        for (size_t i = 0; i < batch_count; i++) {
            size_t current_batch_size;
            if (!remainder_present || i < batch_count - 1) {
                current_batch_size = batch_size;
            } else {
                current_batch_size = total_token_count % batch_size;
            }
            batch.n_tokens = current_batch_size;


            const size_t batch_start = token_idx;
            const size_t batch_end = batch_start + current_batch_size;

            memcpy(batch.token, tokens + batch_start, current_batch_size * sizeof(llama_token));

            while (token_idx < batch_end) {
                if (token_idx == sequence_end) {
                    sequence_idx++;
                    sequence_end += token_counts[sequence_idx];
                }
                if (sequence_idx > INT32_MAX) {
                    LOG_ERROR("Number overflow");
                    ERROR_CLEANUP();
                    return -1;
                }
                /* Current token position within batch */
                size_t relative_token_idx = token_idx - batch_start;
                batch.seq_id[relative_token_idx][0] = sequence_idx;
                token_idx++;
            }
            if (llama_decode(context, batch) != 0) {
                LOG_ERROR("Decode failed");
                ERROR_CLEANUP();
                return -1;
            }
        }
        /* Free batch */
        llama_batch_free(batch);
#undef ERROR_CLEANUP
#define ERROR_CLEANUP() \
        do { \
            free(tokens); \
            free(token_counts); \
            free(embedding_memory); \
        } while (false)
    }
        
    /* Copy embeddings */
    {
        size_t sequence_idx = 0;
        size_t embedding_offset = 0;
        while (sequence_idx < text_count) {
            float *embedding = llama_get_embeddings_seq(context, sequence_idx);
            memcpy(embedding_memory + embedding_offset, embedding, dimension_count * sizeof(float));
            sequence_idx++;
            embedding_offset += dimension_count;
        }
        llama_memory_t context_memory = llama_get_memory(context);
        llama_memory_clear(context_memory, true);
    }
    
    free(tokens);
    free(token_counts);
    embeddings->embeddings = embedding_memory;
    embeddings->dimension_count = dimension_count;
    embeddings->embedding_count = text_count;
#undef ERROR_CLEANUP
    return 0;
}


int init_llama_embedder(
        struct embedder *embedder, 
        struct llama_context *context, 
        size_t n_threads, 
        size_t batch_size)
{
    struct embedder_data *data = malloc(sizeof(struct embedder_data));
    if (data == NULL) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    data->context = context;
    data->n_threads = n_threads;
    data->batch_size = batch_size;
    embedder->data = data;
    embedder->get_embeddings = &get_embeddings;
    embedder->dispose = &dispose_embedder;
}
