/* Structs for polymorphic text embedding. For more information, see embedder_wrapper_functions.h.
 */

#ifndef SMALLRAG_EMBEDDER_CORE_H
#define SMALLRAG_EMBEDDER_CORE_H

#include <stddef.h>

/* A struct representing vector embeddings.
 *
 * An instance's dispose() method must always be called with that instance as its first parameter
 * and must release resources owned by that instance.
 */
struct embeddings {
    float *embeddings;
    size_t dimension_count;
    size_t embedding_count;
    void (*dispose)(struct embeddings *embeddings);
};

/* A struct providing a text-to-vector embedding implementation for polymorphic text embedding.
 *
 * It has two methods. An instance's methods must always be called with that instance as their
 * first parameter.
 *
 * The get_embeddings() method creates vector embeddings for texts.
 *
 * texts[i] represents the i-th text. text_lengths[i] is the i-th text's length. text_count is the
 * total number of texts.
 *
 * The length of each text is required to be greater than 0. So is text_count.
 *
 * embeddings is an out parameter containing the embeddings for all texts.
 *
 * get_embeddings() must not modify texts nor text_lengths.
 *
 * 0 is returned on success and -1 is returned on error. After error, the embeddings out parameter
 * is undefined.
 *
 * The dispose() method releases all resources associated with the embedder.
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

#endif
