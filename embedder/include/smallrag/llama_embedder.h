#ifndef SMALLRAG_LLAMA_EMBEDDER_H
#define SMALLRAG_LLAMA_EMBEDDER_H

#include "smallrag/embedder_core.h"

#include "llama.h"

struct llama_embedder_params {
    size_t n_tokenizer_threads;
    size_t context_size;
    size_t batch_size;
    size_t ubatch_size;
    size_t max_sequences_per_batch;
};

/* Initializes an embedder that uses llama.cpp under the hood.
 *
 * Requires the continued existence of model.
 *
 * Returns 0 on success, -1 on error.
 */
int init_llama_embedder(
        struct embedder *embedder, 
        const struct llama_model *model, 
        const struct llama_embedder_params *params);

#endif
