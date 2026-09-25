#include "smallrag/retrieval.h"
#include "llama.h"

/* Creates a smallrag embedding model from a llama model. The llama model is required to persist throughout the 
 * smallrag model's lifetime.
 */
int smallrag_llama_init_embmod(struct llama_model *llama_model, struct smallrag_embd_model *embmod);



