#ifndef SMALLRAG_LLAMA_EMBEDDER_H
#define SMALLRAG_LLAMA_EMBEDDER_H

#include "smallrag/embedder_core.h"

#include "llama.h"

/* Initializes an embedder that uses llama.cpp under the hood.
 *
 * Returns 0 on success, -1 on error.
 */
int init_llama_embedder(
        struct embedder *embedder, 
        struct llama_context *context, 
        size_t n_threads,
        size_t batch_size);

#endif
