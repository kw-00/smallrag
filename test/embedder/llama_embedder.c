#include "smallrag/llama_embedder.h"
#include "smallrag/embedder.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef MODEL_PATH
#error "MODEL_PATH not defined, though required for tests"
#endif


static void get_embedder(char *model_path, struct embedder* embedder)
{
    struct llama_model_params model_params = llama_model_default_params();
    struct llama_model *model = llama_model_load_from_file(
            model_path, model_params); 

    struct llama_embedder_params params = {
        .n_tokenizer_threads = 2,
        .context_size = 65536,
        .batch_size = 512,
        .ubatch_size = 512,
        .max_sequences_per_batch = 256
    };
    assert(init_llama_embedder(embedder, model, &params) != -1);
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

