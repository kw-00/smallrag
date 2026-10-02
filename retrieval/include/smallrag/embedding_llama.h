#include "smallrag/retrieval.h"
#include "llama.h"

int smallrag_embprov_llama_conf_init(size_t n_tokenizer_threads);

/* Creates a smallrag embedding provider from a llama context. The llama context is required to persist throughout the 
 * smallrag provider's lifetime.
 *
 * passing n_threads of 0 leaves finding the right number to the implementation.
 */
int smallrag_embprov_llama_init(
        struct llama_context *context, 
        size_t n_threads,
        size_t batch_size,
        struct smallrag_embd_provider *embprov);



