#ifndef SMALLRAG_POSTPROCESSING_EMBEDDER_H
#define SMALLRAG_POSTPROCESSING_EMBEDDER_H

#include "smallrag/embedder_core.h"

/* Initializes postprocessing_embedder as a wrapper of wrapped_embedder. postprocessing_embedder
 * behaves just like wrapped_embedder, except when get_embeddings() is used on it, it additionally 
 * runs postprocessing_function on the resulting embeddings before returning them.
 *
 * postprocessing_embedder copies wrapped_embedder and takes ownership of its resources. It will 
 * dispose of wrapped_embedder's resources when disposed.
 *
 * This means that wrapped_embedder should not be used anymore. It also means that the
 * wrapped_embedder struct itself (unlike its resources) may disappear without any consequence
 * for postprocessing_embedder, as postprocessing_embedder copies it.
 */
int init_postprocessing_embedder(
        struct embedder *restrict postprocessing_embedder, 
        struct embedder *restrict wrapped_embedder, 
        int (*postprocessing_function)(struct embeddings *));

#endif
