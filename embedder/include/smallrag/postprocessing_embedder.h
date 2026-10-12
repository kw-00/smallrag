#include "smallrag/embedder_core.h"

/* Initializes postprocessing_embedder as a wrapper of wrapped_embedder. postprocessing_embedder
 * behaves just like wrapped_embedder, except when get_embeddings() is used on it, it additionally 
 * runs postprocessing_function on the resulting embeddings before returning them.
 *
 * postprocessing_embedder takes ownership of wrapped_embedder and disposes it when disposed.
 */
int init_postprocessing_embedder(
        struct embedder *postprocessing_embedder, 
        struct embedder *wrapped_embedder, 
        int (*postprocessing_function)(struct embeddings *));
