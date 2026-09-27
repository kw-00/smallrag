#include "llama.h"
#include "smallrag/err.h"
#include "smallrag/retrieval.h"
#include <stdlib.h>
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
    size_t n_threads;
};

static void* priv_get_embds_task(void *args)
{
    return NULL; // Implement later
}

static int priv_get_embds(
        const struct smallrag_embd_provider *provider, 
        const size_t fragm_cnt,
        const size_t *fragm_sizes, 
        const char **fragments, 
        struct smallrag_embds *embds)
{
    struct priv_provider_data *provider_data = (struct priv_provider_data*)provider->provider_data;
    const struct llama_context *ctx = provider_data->ctx;
    const ssize_t n_threads = provider_data->n_threads;

    const enum llama_pooling_type pooling = llama_pooling_type(ctx);
    if (pooling != LLAMA_POOLING_TYPE_MEAN) {
        smallrag_pusherrmsg(
                smallragERR_UNSUPPORTED, "Only LLAMA_POOLING_TYPE_MEAN is currently supported");
        return -1;
    }
    const struct llama_model *model = llama_get_model(ctx);
    const struct llama_vocab *vocab = llama_model_get_vocab(model);
    size_t frag_cnts[n_threads];
    size_t thread_idx = 0;
    size_t frag_idx = 0;
    while (frag_idx++ < fragm_cnt) {
        frag_cnts[thread_idx]++;
        if (thread_idx == n_threads - 1) {
            thread_idx = 0;
        } else {
            thread_idx++;
        }
    }
    return -1; // Implement later
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
        struct smallrag_embd_provider *embprov)
{
    embprov->ops = &llama_embprov_ops;
    struct priv_provider_data *data = malloc(sizeof(struct priv_provider_data));
    data->ctx = llama_context;
    data->n_threads = n_threads;
    embprov->provider_data = data;
    return 0;
}
