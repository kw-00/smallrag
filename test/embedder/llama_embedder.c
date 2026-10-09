#include "smallrag/llama_embedder.h"
#include "smallrag/embedder.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef MODEL_PATH
#error "MODEL_PATH not defined, though required for tests"
#endif

static struct llama_context *get_context(char *model_path) 
{
    struct llama_model_params model_params = llama_model_default_params();
    struct llama_model *model = llama_model_load_from_file(
            model_path, model_params); 
    struct llama_context_params context_params = llama_context_default_params();
    context_params.n_ctx = 65536;
    context_params.n_batch = 512;
    context_params.embeddings = true;
    context_params.n_seq_max = 256;
    struct llama_context *context = llama_init_from_model(model, context_params);
    return context;
}

static void get_embedder(char *model_path, struct embedder* embedder)
{
    struct llama_context *context = get_context(model_path);
    assert(init_llama_embedder(embedder, context, 2, 512) != -1);
}


static void test_embedding_counts(char **strings, size_t string_count, struct embedder *embedder)
{
    size_t *string_lengths;
    {
        size_t alloc_size;
        assert(!__builtin_mul_overflow(sizeof(size_t), string_count, &alloc_size) && "Number overflow");
        string_lengths = malloc(alloc_size);
        assert(string_lengths != NULL);
    }
    for (size_t i = 0; i < string_count; i++) {
        string_lengths[i] = strlen(strings[i]);
    }
    struct embeddings embeddings;
    assert(get_embeddings(
                embedder, 
                strings, 
                string_lengths, 
                string_count, 
                &embeddings) != -1 && "Embedding ended in an error");
}

int main(void) 
{
    struct embedder embedder;
    get_embedder(MODEL_PATH, &embedder);
    {
        char *strings[3];
        strings[0] = "Hi, my name is Emma.";
        strings[1] = "Emma Bed. You can call me Emm Bed for short.";
        strings[2] = "Nice to meet you, Emm.";
        test_embedding_counts(strings, 3, &embedder);
    }
}

