#include "llama.h"
#include "smallrag/err.h"
#include "smallrag/retrieval.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/param.h>
/*
llama_init_from_model()
llama_model_default_params()
llama_context_default_params()
llama_model_load_from_file()
llama_free()
llama_model_free()
llama_model_get_vocab()
*/


struct priv_provider_data {
    struct llama_context *ctx;
    size_t batch_size;
    size_t n_threads;
};

struct priv_tokenization_task_context {
    const char **fragms; /* All text fragments being processed */
    const size_t *fragm_sizes; /* Sizes of all text fragments */
    size_t fragm_cnt; /* Total number of text fragments */
    llama_token **tokens; /* Output tokens */
    size_t *token_cnts; /* Output token counts (1 per text fragment) */
    const struct llama_vocab *vocab; /* Vocab/tokenizer */
};

static const int priv_task_fail = -1;
static const int priv_task_success = 0;

static void* priv_tokenization_task(void *args)
{
    const struct priv_tokenization_task_context *ctx 
        = (struct priv_tokenization_task_context *)args;
    for (size_t i = 0; i < ctx->fragm_cnt; i++) {
        if (ctx->fragm_sizes[i] > INT32_MAX) {
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return (void *)&priv_task_fail;
        }
        if (ctx->fragm_sizes[i] == 0) {
            ctx->token_cnts[i] = 0;
            continue;
        }
        int32_t token_count = llama_tokenize(
                ctx->vocab,
                ctx->fragms[i],
                ctx->fragm_sizes[i],
                ctx->tokens[i],
                ctx->fragm_sizes[i],
                true,
                true);
        if (token_count < 0) {
            return (void *)&priv_task_fail;
        }
        ctx->token_cnts[i] = token_count;
    }
    return (void *)&priv_task_success;
}

struct priv_mean_task_context {
    const float *embeddings; /* Source embeddings, 1 per every tokeni in every text fragment */
    size_t fragm_cnt; /* Total text fragment count */
    const size_t *vec_cnts; /* Number of vectors for each fragment */
    size_t vec_size; /* Embedding vector size (same for all vectors) */
    float *out; /* Output vectors, 1 per text fragment */
};

static void *priv_mean_task(void *args)
{
    const struct priv_mean_task_context *ctx = (struct priv_mean_task_context *)args;
    const float *embeddings = ctx->embeddings;
    const size_t fragm_cnt = ctx->fragm_cnt;
    const size_t vec_size = ctx->vec_size;
    float *const out = ctx->out;

    size_t fragm_vecs_offset = 0;
    for (size_t fragm_idx = 0; fragm_idx < fragm_cnt; fragm_idx++) {
        size_t fragm_vec_cnt = ctx->vec_cnts[fragm_idx];
        for (size_t el_idx = 0; el_idx < vec_size; el_idx++) {
            size_t out_el_index = fragm_idx * vec_size + el_idx;
            for (size_t vec_idx = 0; vec_idx < fragm_vec_cnt; vec_idx++) {
                size_t abs_el_index = fragm_vecs_offset + vec_idx * vec_size + el_idx;
                out[out_el_index] += embeddings[abs_el_index];
            }
            out[out_el_index] /= fragm_vec_cnt;
        }
        fragm_vecs_offset += fragm_vec_cnt * vec_size;
    }
    return (void *)&priv_task_success;
}

static int priv_get_embds(
        const struct smallrag_embd_provider *provider, 
        size_t full_text_size,
        size_t fragm_cnt,
        const size_t *fragm_sizes, 
        const char **fragments, 
        struct smallrag_embds *embds)
{
    if (full_text_size == 0) {
        smallrag_pusherrmsg(smallragERR_ARGUMENT, "full_text_size is 0");
        return -1;
    }
    if (fragm_cnt == 0) {
        smallrag_pusherrmsg(smallragERR_ARGUMENT, "fragm_cnt is 0");
        return -1;
    }

    
    /* ----------------------------------------------------------------------------------------- */ 
    // Extract type-erased data from embd_provider
    /* ----------------------------------------------------------------------------------------- */ 
    const struct priv_provider_data *provider_data = (struct priv_provider_data*)provider->provider_data;

    // llama_context
    struct llama_context *ctx = provider_data->ctx;

    // n_threads
    size_t n_threads;
    if (provider_data->n_threads > 0) {
         n_threads = provider_data->n_threads;
    } else {
        long n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        if (n_cpus > SIZE_MAX) {
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        n_threads = n_cpus;
    }

    // Decode batch size — tokens per batch
    const size_t batch_size = provider_data->batch_size;
    if (batch_size > INT32_MAX) {
        smallrag_pusherrmsg(smallragERR_NUM_OVERFLOW, "batch_size too large");
        return -1;
    }

    const enum llama_pooling_type pooling = llama_pooling_type(ctx);
    if (pooling != LLAMA_POOLING_TYPE_MEAN) {
        smallrag_pusherrmsg(
                smallragERR_UNSUPPORTED, "Only LLAMA_POOLING_TYPE_MEAN is currently supported");
        return -1;
    }
    

    /* ----------------------------------------------------------------------------------------- */
    // Prepare key llama-related stuff — model, vocab (tokenizer) and embedding size
    /* ----------------------------------------------------------------------------------------- */
    const struct llama_model *model = llama_get_model(ctx);
    // Prepare tokenizer
    const struct llama_vocab *vocab = llama_model_get_vocab(model);
    const int32_t embd_size = llama_model_n_embd(model);
    if (embd_size < 2) {
        smallrag_pusherrmsg(smallragERR_INVALID_STATE, "llama model embedding size was 1 or less.");
        return -1;
    }

    /* ----------------------------------------------------------------------------------------- */ 
    // Prepare memory for thread handles
    /* ----------------------------------------------------------------------------------------- */ 
    pthread_t *thread_handles;
    {
            // Memory for worker handles (classic pthread_t)
            size_t worker_handle_alloc_size;
            if (__builtin_mul_overflow(
                        sizeof(pthread_t), n_threads, &worker_handle_alloc_size)) {
                smallrag_pusherr(smallragERR_NUM_OVERFLOW);
                return -1;
            }
            thread_handles = malloc(worker_handle_alloc_size);
            if (thread_handles == NULL) {
                smallrag_pusherrstd();
                return -1;
            }
    }
#define privERR_CLEANUP() free(thread_handles)
  
    /* ----------------------------------------------------------------------------------------- */ 
    // Prepare memory for tokenization 
    /* ----------------------------------------------------------------------------------------- */ 

    // Memory spans for use in tokenization
    void *tokenization_mem_handle;
    struct priv_tokenization_task_context *tokenization_worker_ctx;
    {
        llama_token *tokens;
        llama_token **token_ptrs;
        size_t *token_cnts;
        {
            size_t tokenization_alloc_size;
            {

                // Calculate memory needs
                size_t tokenization_worker_ctx_alloc_size; 
                if (__builtin_mul_overflow(
                            sizeof(struct priv_tokenization_task_context), n_threads, 
                            &tokenization_worker_ctx_alloc_size)) {
                    privERR_CLEANUP();
                    smallrag_pusherr(smallragERR_NUM_OVERFLOW);
                    return -1;
                }

                // Allocation size for resulting tokens (assumed worst-case scenario, where 1 byte of 
                // text to 1 token)
                size_t token_alloc_size;
                if (__builtin_mul_overflow(sizeof(llama_token), full_text_size, &token_alloc_size)) {
                    privERR_CLEANUP();
                    smallrag_pusherr(smallragERR_NUM_OVERFLOW);
                    return -1;
                }

                // Allocation size for ptrs to first token of each fragment and for fragment token 
                // counts
                const size_t token_metadata_alloc_per_fragm = sizeof(llama_token *) + sizeof(size_t);
                size_t token_metadata_alloc;
                if (__builtin_mul_overflow(
                            token_metadata_alloc_per_fragm, fragm_cnt, 
                            &token_metadata_alloc)) {
                    privERR_CLEANUP();
                    smallrag_pusherr(smallragERR_NUM_OVERFLOW);
                    return -1;
                }

                // Total allocation size for tokenization
                size_t tokenization_alloc_size;
                if (__builtin_add_overflow(
                            tokenization_worker_ctx_alloc_size * n_threads, token_alloc_size, 
                            &tokenization_alloc_size)) {
                    privERR_CLEANUP();
                    smallrag_pusherr(smallragERR_NUM_OVERFLOW);
                    return -1;
                }
                if (__builtin_add_overflow(
                            tokenization_alloc_size, token_metadata_alloc, 
                            &tokenization_alloc_size)) {
                    privERR_CLEANUP();
                    smallrag_pusherr(smallragERR_NUM_OVERFLOW);
                    return -1;
                }
            }
            void *const tokenization_mem = malloc(tokenization_alloc_size);
            if (tokenization_mem == NULL) {
                privERR_CLEANUP();
                smallrag_pusherrstd();
                return -1;
            }
            tokenization_mem_handle = tokenization_mem;
            tokenization_worker_ctx = tokenization_mem;
            tokens = (void *)(tokenization_worker_ctx + n_threads);
            token_ptrs = (void *)(tokens + full_text_size);
            token_cnts = (void *)(token_ptrs + fragm_cnt);
        }
#undef privERR_CLEANUP
#define privERR_CLEANUP() \
    do { \
        free(thread_handles); \
        free(tokenization_mem_handle); \
    } while (false)
        /* ------------------------------------------------------------------------------------- */ 
        // Tokenize text fragments 
        /* ------------------------------------------------------------------------------------- */ 
        {
            // Distribute text fragments evenly across workers
            size_t thread_idx = 0;
            size_t fragm_idx = 0;
            while (fragm_idx++ < fragm_cnt) {
                tokenization_worker_ctx[thread_idx].fragm_cnt++;
                if (thread_idx == n_threads - 1) {
                    thread_idx = 0;
                } else {
                    thread_idx++;
                }
            }
        }
        
        {
            // Launch tokenization workers
            size_t fragm_idx = 0;
            size_t char_idx = 0;
            for (size_t thread_idx = 0; thread_idx < n_threads; thread_idx++) {
                // Initialize worker data
                tokenization_worker_ctx[thread_idx].fragms = fragments + fragm_idx;
                tokenization_worker_ctx[thread_idx].fragm_sizes = fragm_sizes + fragm_idx;
                tokenization_worker_ctx[thread_idx].vocab = vocab;
                tokenization_worker_ctx[thread_idx].tokens = token_ptrs + fragm_idx;
                tokenization_worker_ctx[thread_idx].token_cnts = token_cnts + fragm_idx;

                for (size_t i = 0; i < tokenization_worker_ctx[thread_idx].fragm_cnt; i++) {
                    char_idx += tokenization_worker_ctx[thread_idx].fragm_sizes[i];
                }
                *tokenization_worker_ctx[thread_idx].tokens = tokens + char_idx;

                // Start worker (each worker tokenizes a subset of all fragments)
                int pthread_err = pthread_create(
                        &thread_handles[thread_idx],
                        NULL,
                        &priv_tokenization_task,
                        &tokenization_worker_ctx[thread_idx]);
                if (pthread_err != 0) {
                    privERR_CLEANUP();
                    errno = pthread_err;
                    smallrag_pusherrstd();
                    return -1;
                }
                fragm_idx += tokenization_worker_ctx[fragm_idx].fragm_cnt;
            }
            // Join workers
            for (size_t i = 0; i < n_threads; i++) {
                int *thread_exit;
                if (pthread_join(thread_handles[i], (void **)&thread_exit) == -1) {
                    privERR_CLEANUP();
                    smallrag_pusherrstd();
                    return -1;
                }
                if (*thread_exit == -1) {
                    privERR_CLEANUP();
                    return -1;
                }
            }
        }
    }


    /* ----------------------------------------------------------------------------------------- */
    // Calculate total token count
    /* ----------------------------------------------------------------------------------------- */
    size_t n_tokens = 0;
    {
        for (size_t i = 0; i < n_threads; i++) {
            struct priv_tokenization_task_context *w_ctx = tokenization_worker_ctx + i;
            for (size_t j = 0; j < w_ctx->fragm_cnt; j++) {
                n_tokens += w_ctx->token_cnts[j];
            }
        } 
    }

    
    /* ----------------------------------------------------------------------------------------- */ 
    // Prepare memory for decode
    /* ----------------------------------------------------------------------------------------- */ 
    void *decode_mem_handle;
    llama_token *tokens;
    size_t *token_cnts;
    float *embeddings;
    {
            size_t token_alloc_size;
        if (__builtin_mul_overflow(sizeof(llama_token) + sizeof(size_t), n_tokens, &token_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        const size_t n_embeddings = n_tokens / batch_size 
            + (n_tokens % batch_size > 0 /* For ceil division */);    
        size_t embd_alloc_size;
        if (__builtin_mul_overflow(sizeof(float), embd_size, &embd_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        if (__builtin_mul_overflow(embd_alloc_size, n_tokens, &embd_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }

        size_t full_alloc_size;
        if (__builtin_add_overflow(token_alloc_size, embd_alloc_size, &full_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        void *const mem = malloc(full_alloc_size);
        if (mem == NULL) {
            privERR_CLEANUP();
            smallrag_pusherrstd();
            return -1;
        }
        decode_mem_handle = mem;
        tokens = mem;
        token_cnts = (void *)(tokens + n_tokens);
        embeddings = (void *)(token_cnts + fragm_cnt);
    }


    /* ----------------------------------------------------------------------------------------- */ 
    // Move all tokens to a contiguous memory span for easier batching and also copy token
    // count data for each fragment
    /* ----------------------------------------------------------------------------------------- */ 
    {
        size_t token_idx = 0;
        size_t fragm_idx = 0;
        for (size_t i = 0; i < n_threads; i++) {
            const struct priv_tokenization_task_context *w_ctx = tokenization_worker_ctx + i;
            for (size_t j = 0; j < w_ctx->fragm_cnt; j++) {
                memcpy(tokens + token_idx, w_ctx->tokens[j], w_ctx->token_cnts[j]);
                token_idx += w_ctx->token_cnts[j];
            }
            memcpy(token_cnts + fragm_idx, w_ctx->token_cnts, w_ctx->fragm_cnt);
            fragm_idx += w_ctx->fragm_cnt;
        }
    }
    // Free memory used for tokenization — tokens have been copied, it isn't needed anymore
    free(tokenization_mem_handle);
#undef privERR_CLEANUP
#define privERR_CLEANUP() free(thread_handles)


    /* ----------------------------------------------------------------------------------------- */ 
    // Prepare batches and decode tokens
    /* ----------------------------------------------------------------------------------------- */ 
    {
        size_t n_batches = n_tokens / batch_size + (n_tokens % batch_size > 0);
        
        size_t fragm_idx = 0;
        size_t tok_idx = 0;
        size_t fragm_tks_left = token_cnts[0];
        for (size_t i = 0; i < n_batches; i++) {
            int32_t actual_batch_size;
            if (i < n_batches - 1) {
                actual_batch_size = batch_size;
            } else {
                actual_batch_size = n_tokens % batch_size;
            }

            const struct llama_batch batch = llama_batch_init(
                    actual_batch_size,
                    embd_size,
                    1);
            size_t batch_tks_left = actual_batch_size;
            // Batch and assign seq ID
            while (true) {
                const size_t current_token = actual_batch_size - batch_tks_left;
                for (size_t j = current_token; 
                        j < MIN(actual_batch_size, current_token + fragm_tks_left); j++) {
                    batch.seq_id[j][0] = fragm_idx;
                }
                if (fragm_tks_left > batch_tks_left) {
                    fragm_tks_left -= batch_tks_left;
                    break;
                }
                if (fragm_tks_left == batch_tks_left) {
                    fragm_tks_left = token_cnts[++fragm_idx];
                    break;
                }
                batch_tks_left -= fragm_tks_left;
                fragm_tks_left = token_cnts[++fragm_idx];
                continue;
            }
            // Decode
            const int32_t decode_status = llama_decode(ctx, batch);
            if (decode_status != 0) {
                privERR_CLEANUP();
                smallrag_pusherr(smallragERR_UNEXPECTED);
                return -1;
            }
            // Save embeddings
            memcpy(embeddings + tok_idx, batch.embd, actual_batch_size);
            tok_idx += actual_batch_size;
        }
    }
    
    /* ----------------------------------------------------------------------------------------- */
    // Prepare memory for mean pooling
    /* ----------------------------------------------------------------------------------------- */
    struct priv_mean_task_context *mean_task_ctx;
    {
        size_t task_ctx_alloc_size;
        if (__builtin_mul_overflow(
                    sizeof(struct priv_mean_task_context), n_threads, &task_ctx_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }

        mean_task_ctx = malloc(task_ctx_alloc_size);
        if (mean_task_ctx == NULL) {
            privERR_CLEANUP();
            smallrag_pusherrstd();
            return -1;
        }
    }
#undef privERR_CLEANUP
#define privERR_CLEANUP() \
    do { \
        free(thread_handles); \
        free(mean_task_ctx); \
    } while (false)
    
    /* ----------------------------------------------------------------------------------------- */
    // Prepare memory for embedding struct and the embeddings it points to. This memory will not
    // be freed unless an error occurrs, as it belongs to the function's out parameter
    /* ----------------------------------------------------------------------------------------- */
    struct smallrag_embds *embds_struct;
    float *mean_embds;
    {
        size_t embedding_alloc_size;
        if (__builtin_mul_overflow(sizeof(float), embd_size, &embedding_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        if (__builtin_mul_overflow(embedding_alloc_size, fragm_cnt, &embedding_alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        size_t alloc_size;
        if (__builtin_add_overflow(sizeof(struct smallrag_embds), embedding_alloc_size, &alloc_size)) {
            privERR_CLEANUP();
            smallrag_pusherr(smallragERR_NUM_OVERFLOW);
            return -1;
        }
        void *mem = malloc(alloc_size);
        if (mem == NULL) {
            privERR_CLEANUP();
            smallrag_pusherrstd();
            return -1;
        }
        embds_struct = mem;
        mean_embds = (void *)(embds_struct + 1);
    }
#undef privERR_CLEANUP
#define privERR_CLEANUP() \
    do { \
        free(thread_handles); \
        free(mean_task_ctx); \
        free(embds_struct); \
    } while (false)
    
    {
        size_t base_fragms_per_thread = fragm_cnt / n_threads;
        size_t leftover = fragm_cnt % n_threads;
        size_t fragm_offset = 0;
        for (size_t i = 0; i < n_threads; i++) {
            size_t n_fragms = base_fragms_per_thread + (i < leftover);
            if (n_fragms == 0) break;
            mean_task_ctx[i] = (struct priv_mean_task_context){
                .embeddings = embeddings + fragm_offset,
                .fragm_cnt = n_fragms,
                .vec_cnts = token_cnts + fragm_offset,
                .vec_size = embd_size,
                .out = mean_embds + fragm_offset * embd_size
            };
            int pthread_err = pthread_create(
                    thread_handles + i,
                    NULL,
                    &priv_mean_task,
                    mean_task_ctx + i);
            if (pthread_err != 0) {
                privERR_CLEANUP();
                errno = pthread_err;
                smallrag_pusherrstd();
                return -1;
            }
            fragm_offset += n_fragms;
        }
    }
    // Free memory that is not owned by out param
    free(thread_handles);
    free(mean_task_ctx);

    // Make embds_struct point to the text embedding vectors
    *embds_struct = (struct smallrag_embds){
        .vecs = mean_embds,
        .dim = embd_size,
        .cnt = fragm_cnt
    };
    // Set out param to reference resulting embeddings
    *embds = *embds_struct;
    return 0;
#undef privERR_CLEANUP
}

static void priv_free_provider(struct smallrag_embd_provider *provider)
{
    struct priv_provider_data *data = (struct priv_provider_data *)provider->provider_data;
    llama_free(data->ctx);
    free(data);
}

struct smallrag_embd_provider_ops llama_embprov_ops = {
    .get_embds = &priv_get_embds,
    .free = &priv_free_provider
};

int smallrag_embprov_llama_init(
        struct llama_context *llama_context, 
        size_t n_threads,
        size_t batch_size,
        struct smallrag_embd_provider *embprov)
{
    embprov->ops = &llama_embprov_ops;
    struct priv_provider_data *data = malloc(sizeof(struct priv_provider_data));
    data->ctx = llama_context;
    data->n_threads = n_threads;
    data->batch_size = batch_size;
    embprov->provider_data = data;
    return 0;
}
