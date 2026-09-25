#include "smallrag/retrieval.h"
#include "llama.h"

/* Creates a smallrag embedding provider from a llama context. The llama context is required to persist throughout the 
 * smallrag provider's lifetime.
 */
int smallrag_llama_init_embprov(struct llama_context *context, struct smallrag_embd_provider *embprov);



