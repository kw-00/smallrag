#ifndef SMALLRAG_RETRIEVAL_LLAMA_H
#define SMALLRAG_RETRIEVAL_LLAMA_H

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

static int priv_get_embds(const struct smallrag_embd_provider *provider, const struct smallrag_text_frags *frags, struct smallrag_embds *embds)
{
    struct llama_context *ctx = (struct llama_context *)provider->provider_data;
    const enum llama_pooling_type pooling = llama_pooling_type(ctx);
    if (pooling != LLAMA_POOLING_TYPE_MEAN) {
        smallrag_pusherrmsg(smallragERR_UNSUPPORTED, "Only LLAMA_POOLING_TYPE_MEAN is currently supported");
        return -1;
    }
    const struct llama_model *model = llama_get_model(ctx);
    const struct llama_vocab *vocab = llama_model_get_vocab(model);
    const struct llama_



}

static void priv_free_provider(struct smallrag_embd_provider *provider)
{
    const struct llama_context *ctx = (struct llama_context *)provider->provider_data;
    llama_free(ctx);
}

struct smallrag_embd_provider_ops llama_embprov_ops = {
    .get_embds = &priv_get_embds,
    .free = &priv_free_provider
};

int smallrag_llama_init_embprov(struct llama_model *llama_model, struct smallrag_embd_provider *embprov)
{
    embprov->ops = &llama_embprov_ops;
    embprov->provider_data = llama_model;
    return 0;
}

#endif
