#include "smallrag/llama_embedder.h"
#include "smallrag/embedder.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#ifndef GGUF_MODEL_PATHS
#error "GGUF_MODEL_PATHS expected"
#endif

enum { TEST_BATCH_SIZE = 1024, TEST_MAX_SEQUENCES = 256 };

static struct llama_model *get_embedder(char *model_path, struct embedder* embedder)
{
    struct llama_model_params model_params = llama_model_default_params();
    struct llama_model *model = llama_model_load_from_file(
            model_path, model_params); 
    assert(model != NULL);

    struct llama_embedder_params params = {
        .n_tokenizer_threads = 2,
        .batch_size = TEST_BATCH_SIZE,
        .max_sequences_per_batch = TEST_MAX_SEQUENCES
    };
    assert(init_llama_embedder(embedder, model, &params) != -1);
    return model;
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
    assert(embeddings.embeddings != NULL && embeddings.dispose != NULL);
    assert(embeddings.embedding_count == string_count);
    assert(embeddings.dimension_count > 0);
    embeddings.dispose(&embeddings);
    free(string_lengths);
}

static size_t count_tokens(struct llama_model *model, const char *text, size_t length)
{
    assert(length <= INT32_MAX);
    int32_t result = llama_tokenize(
            llama_model_get_vocab(model), text, (int32_t)length, NULL, 0, true, true);
    assert(result < 0 && result != INT32_MIN);
    return (size_t)-result;
}

static void test_large_batch(struct llama_model *model, struct embedder *embedder)
{
    char text[] = "Hi, my name is Emma.";
    size_t tokens_per_text = count_tokens(model, text, strlen(text));
    assert(tokens_per_text >= 3 && tokens_per_text <= TEST_BATCH_SIZE / 2);

    size_t string_count = TEST_BATCH_SIZE / 2 / tokens_per_text + 1;
    assert(string_count <= TEST_MAX_SEQUENCES);
    assert(string_count * tokens_per_text <= TEST_BATCH_SIZE);
    char **strings = malloc(string_count * sizeof(*strings));
    assert(strings != NULL);
    for (size_t i = 0; i < string_count; i++) {
        strings[i] = text;
    }
    test_embedding_counts(strings, string_count, embedder);
    free(strings);
}

static void test_oversized_sequence(struct llama_model *model, struct embedder *embedder)
{
    const char fragment[] = "Hi, my name is Emma. ";
    size_t fragment_length = sizeof(fragment) - 1;
    size_t repetitions = 128;
    char *text;
    size_t length;
    while (true) {
        length = fragment_length * repetitions;
        text = malloc(length + 1);
        assert(text != NULL);
        for (size_t i = 0; i < repetitions; i++) {
            memcpy(text + i * fragment_length, fragment, fragment_length);
        }
        text[length] = '\0';
        if (count_tokens(model, text, length) > TEST_BATCH_SIZE) {
            break;
        }
        free(text);
        repetitions *= 2;
        assert(repetitions <= 16384);
    }

    bool tested_long_sequence = false;
    for (size_t i = 1; i <= repetitions; i++) {
        size_t prefix_length = i * fragment_length;
        size_t token_count = count_tokens(model, text, prefix_length);
        if (token_count > TEST_BATCH_SIZE / 2) {
            assert(token_count <= TEST_BATCH_SIZE);
            char saved = text[prefix_length];
            text[prefix_length] = '\0';
            char *strings[] = {text};
            test_embedding_counts(strings, 1, embedder);
            text[prefix_length] = saved;
            tested_long_sequence = true;
            break;
        }
    }
    assert(tested_long_sequence);

    char *strings[] = {text};
    size_t string_lengths[] = {length};
    struct embeddings embeddings;
    assert(get_embeddings(embedder, strings, string_lengths, 1, &embeddings) == -1);
    free(text);
}

int main(void) 
{
    fprintf(stderr, "PATHS: %s\n", GGUF_MODEL_PATHS);
    char model_paths[] = GGUF_MODEL_PATHS;
    char *saveptr = NULL;
    char *path = strtok_r(model_paths, ",", &saveptr);
    while (path != NULL) {
        struct embedder embedder;
        struct llama_model *model = get_embedder(path, &embedder);
        {
            char *strings[3];
            strings[0] = "Hi, my name is Emma.";
            strings[1] = "Emma Bed. You can call me Emm Bed for short.";
            strings[2] = "Nice to meet you, Emm.";
            test_embedding_counts(strings, 3, &embedder);
        }
        test_large_batch(model, &embedder);
        test_oversized_sequence(model, &embedder);
        dispose_embedder(&embedder);
        llama_model_free(model);
        path = strtok_r(NULL, ",", &saveptr);
    }
}
