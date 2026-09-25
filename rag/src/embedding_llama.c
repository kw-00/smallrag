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

static int priv_get_embds(const struct smallrag_embd_model *model, const struct smallrag_text_frags *frags, struct smallrag_embds *embds)
{
    return -1;
}

static void priv_free_model(struct smallrag_embd_model *model)
{
    struct llama_model *intern_model = (struct llama_model *)model->model_data;
    llama_model_free(intern_model);
}

struct smallrag_embd_model_ops llama_embmod_ops = {
    .get_embds = &priv_get_embds,
    .free = &priv_free_model
};

int smallrag_llama_init_embmod(struct llama_model *llama_model, struct smallrag_embd_model *embmod)
{
    embmod->ops = &llama_embmod_ops;
    embmod->model_data = llama_model;
    return 0;
}

#endif
