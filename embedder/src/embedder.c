#include "smallrag/embedder.h"

int get_embeddings(
        struct embedder *embedder,
        const char **texts,
        const size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings)
{
    return embedder->get_embeddings(embedder, texts, text_lengths, text_count, embeddings);
}


void dispose_embedder(struct embedder *embedder)
{
    embedder->dispose(embedder);
}

