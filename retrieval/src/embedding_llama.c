#include "llama.h"
#include "smallrag/retrieval.h"
#include "smallrag/err.h"
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
    struct smallrag_embprov_llama_conf *conf;
} 

static void* priv_get_embds_task(void *args)
{
    struct smallrag_text_frags *frags = (struct smallrag_text_frags *)args;

    for (int i = 0; i < frags->count; i++) {


}

static int priv_get_embds(const struct smallrag_embd_provider *provider, const struct smallrag_text_frags *frags, struct smallrag_embds *embds)
{
    struct priv_provider_data *provider_data = (struct priv_provider_data*)provider->provider_data;
    const llama_context *ctx = provider_data->ctx;
    const smallrag_embprov_llama_conf *cont = provider_data->conf;

    const enum llama_pooling_type pooling = llama_pooling_type(ctx);
    if (pooling != LLAMA_POOLING_TYPE_MEAN) {
        smallrag_pusherrmsg(smallragERR_UNSUPPORTED, "Only LLAMA_POOLING_TYPE_MEAN is currently supported");
        return -1;
    }
    const struct llama_model *model = llama_get_model(ctx);
    const struct llama_vocab *vocab = llama_model_get_vocab(model);
    size_t frag_cnts[conf->n_tokenizer_threads];
    size_t thread_idx = 0;
    size_t frag_idx = 0;
    while (frag_idx++ < frags->count) {
        frag_cnts[thread_idx]++;
        if (thread_idx == conf->n_tokenizer_threads - 1) {
            thread_idx = 0;
        } else {
            thread_idx++;
        }
    }
    struct smallrag_text_frags *frag_sets = malloc(sizeof(struct smallrag_text_frags) * conf->n_tokenizer_threads);
    size_t offset = 0;
    for (int i = 0; i < conf->n_tokenizer_threads; i++) {
        frag_sets[i]->frags = frags[offset];
        frag_sets[i]->count = frag_cnts[i];
        offset += frag_cnts[i];
    }



}

static void priv_free_provider(struct smallrag_embd_provider *provider)
{
    const struct priv_provider_data *data = (struct priv_provider_data *)provider->provider_data;
    llama_free(data->ctx);
    free(data);
}

struct smallrag_embd_provider_ops llama_embprov_ops = {
    .get_embds = &priv_get_embds,
    .free = &priv_free_provider
};


int smallrag_embprov_llama_conf_init(size_t n_tokenizer_threads, smallrag_embprov_llama_conf *conf)
{
    if (n_tokenizer_threads == 0) {
        smallrag_pusherrmsg(smallragERR_ARGUMENT, "n_tokenizer_threads must be greater than 0");
        return -1;
    }
    conf->n_tokenizer_threads = n_tokenizer_threads;
}

int smallrag_init_embprov_llama(struct llama_context *llama_context, struct smallrag_embprov_llama_conf *cont, struct smallrag_embd_provider *embprov)
{
    embprov->ops = &llama_embprov_ops;
    struct priv_provider_data *data = malloc(sizeof(struct priv_provider_data));
    data->ctx = llama_context;
    data->conf = cont;
    embprov->provider_data = data;
    return 0;
}

