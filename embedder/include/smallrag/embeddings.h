#ifndef SMALLRAG_EMBEDDINGS_H
#define SMALLRAG_EMBEDDINGS_H

#include "smallrag/embedder_core.h"

/* Copies and concatenates the embeddings pointed to by input into the struct pointed to by output.
 * input_count is the number of input structs.
 *
 * input_count must be a positive number — 0 is not allowed.
 *
 * Does not take ownership of input or its resources.
 *
 * Returns 0 on success, -1 on failure.
 */
int concat_embeddings(
        struct embeddings **input,
        size_t input_count,
        struct embeddings *output);

#endif
