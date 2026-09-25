#ifndef SMALLRAG_RETRIEVAL_LLAMA_H
#define SMALLRAG_RETRIEVAL_LLAMA_H

#include "llama.h"
#include "smallrag/retrieval.h"
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
    struct llama_model *model;
    struct llama_context *context;
};

static int priv_get_embds(const struct smallrag_embd_provider *provider, const struct smallrag_text_frags *frags, struct smallrag_embds *embds)
{
    return -1;
}

static void priv_free_provider(struct smallrag_embd_provider *provider)
{
    struct priv_provider_data *intern_prov = (struct priv_provider_data*)provider->provider_data;
    struct llama_model *model = intern_prov->model;
    struct llama_context *context = intern_prov->context;
    llama_model_free(model);
    llama_free(context);
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
