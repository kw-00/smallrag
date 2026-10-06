#ifndef SMALLRAG_EMBEDDER_WRAPPER_FUNCTIONS_H
#define SMALLRAG_EMBEDDER_WRAPPER_FUNCTIONS_H

#include "smallrag/embedder_core.h"

/* Uses an embedder to provide vector embeddings for a given set of text chunks.
 * Calls the embedder's get_embeddings() method to achieve this.
 *
 * texts[i] is the i-th text chunk.
 * text_lengths[i] is that chunk's size in bytes.
 * text_count is the number of chunks.
 *
 * embeddings is an out parameter that provides the resulting embeddings.
 *
 * Returns 0 on success, -1 on error.
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
