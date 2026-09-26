#include "smallrag/retrieval.h"
#include "llama.h"

struct smallrag_embprov_llama_conf {
    size_t n_tokenizer_threads;
}

int smallrag_embprov_llama_conf_init(size_t n_tokenizer_threads);

/* Creates a smallrag embedding provider from a llama context. The llama context is required to persist throughout the 
 * smallrag provider's lifetime.
 */
int smallrag_init_embprov_llama(struct llama_context *context, struct smallrag_embprov_llama_conf *conf, struct smallrag_embd_provider *embprov);



