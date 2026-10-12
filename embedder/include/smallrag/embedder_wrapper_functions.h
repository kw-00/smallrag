/* Functions for polymorphic text embedding using embedders. For more information about embedders
 * themselves, check embedder_core.h.
 */

#ifndef SMALLRAG_EMBEDDER_WRAPPER_FUNCTIONS_H
#define SMALLRAG_EMBEDDER_WRAPPER_FUNCTIONS_H

#include "smallrag/embedder_core.h"

/* Function wrapper for embedder's get_embeddings() method.
 */
int get_embeddings(
        struct embedder *embedder,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings);

/* Releases resources owned by an embedder using the embedder's dispose() method.
 */
void dispose_embedder(struct embedder *embedder);

#endif
