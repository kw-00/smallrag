#include <stddef.h>

struct embeddings {
    float *embeddings;
    size_t dimension_count;
    size_t embedding_count;
    void (*dispose)(struct embeddings *embeddings);
};

/* A struct providing a text-to-vector embedding implementation for polymorphic text embedding.
 * 
 * See embedder_wrapper_functions.h for more information on how the get_embeddings() and dispose()
 * methods work.
 */
struct embedder {
    int (*get_embeddings)(
            struct embedder *embedder,
            char **texts,
            size_t *text_lengths,
            size_t text_count,
            struct embeddings *embeddings);
    void (*dispose)(struct embedder *embedder);
    void *data;
};
