#include "smallrag/embeddings.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


static size_t input_dispose_count;


static void dispose_input(struct embeddings *embeddings)
{
    input_dispose_count++;
    free(embeddings->embeddings);
    free(embeddings);
}


static struct embeddings *make_input(const float *values, size_t count, size_t dimensions)
{
    struct embeddings *input = malloc(sizeof(*input));
    assert(input != NULL);
    input->embeddings = malloc(count * dimensions * sizeof(float));
    assert(input->embeddings != NULL);
    memcpy(input->embeddings, values, count * dimensions * sizeof(float));
    input->embedding_count = count;
    input->dimension_count = dimensions;
    input->dispose = &dispose_input;
    return input;
}


static void test_concat(void)
{
    const float first_values[] = {1, 2, 3, 4, 5, 6};
    const float second_values[] = {7, 8, 9};
    const float expected[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    struct embeddings *first = make_input(first_values, 2, 3);
    struct embeddings *second = make_input(second_values, 1, 3);
    struct embeddings *input[] = {first, second};
    size_t previous_dispose_count = input_dispose_count;

    struct embeddings *output = NULL;
    assert(concat_embeddings(input, 2, &output) == 0);
    assert(input[0] == first && input[1] == second);
    assert(input_dispose_count == previous_dispose_count);
    assert(output->embedding_count == 3 && output->dimension_count == 3);
    assert(output->embeddings != first->embeddings && output->embeddings != second->embeddings);
    assert((uintptr_t)output->embeddings % _Alignof(float) == 0);
    assert(memcmp(output->embeddings, expected, sizeof(expected)) == 0);
    assert(memcmp(first->embeddings, first_values, sizeof(first_values)) == 0);
    assert(memcmp(second->embeddings, second_values, sizeof(second_values)) == 0);

    first->embeddings[0] = 100;
    assert(output->embeddings[0] == 1);
    output->embeddings[6] = 200;
    assert(second->embeddings[0] == 7);
    output->embeddings[6] = 7;
    first->dispose(first);
    second->dispose(second);
    assert(input_dispose_count == previous_dispose_count + 2);
    assert(memcmp(output->embeddings, expected, sizeof(expected)) == 0);
    output->dispose(output);
}


static void test_single_and_repeated_input(void)
{
    const float values[] = {1, 2};
    const float expected[] = {1, 2, 1, 2};
    struct embeddings *source = make_input(values, 1, 2);
    struct embeddings *input[] = {source};
    struct embeddings *first;
    assert(concat_embeddings(input, 1, &first) == 0);

    struct embeddings *repeated[] = {first, first};
    struct embeddings *second;
    assert(concat_embeddings(repeated, 2, &second) == 0);
    assert(second->embedding_count == 2 && second->dimension_count == 2);
    first->dispose(first);
    assert(memcmp(source->embeddings, values, sizeof(values)) == 0);
    source->dispose(source);
    assert(memcmp(second->embeddings, expected, sizeof(expected)) == 0);
    second->dispose(second);
}


static void test_empty(void)
{
    struct embeddings *output;
    assert(concat_embeddings(NULL, 0, &output) == 0);
    assert(output->embeddings == NULL);
    assert(output->embedding_count == 0 && output->dimension_count == 0);
    output->dispose(output);

    struct embeddings empty = {0};
    struct embeddings *empty_input[] = {&empty};
    assert(concat_embeddings(empty_input, 1, &output) == 0);
    assert(output->embedding_count == 0);
    output->dispose(output);

    float values[] = {1, 2};
    struct embeddings source = {
        .embeddings = values,
        .embedding_count = 1,
        .dimension_count = 2
    };
    struct embeddings *input[] = {&empty, &source, &empty};
    assert(concat_embeddings(input, 3, &output) == 0);
    assert(output->embedding_count == 1 && output->dimension_count == 2);
    assert(memcmp(output->embeddings, values, sizeof(values)) == 0);
    output->dispose(output);
}


static void test_invalid_input(void)
{
    float values[] = {1, 2};
    struct embeddings first = {
        .embeddings = values,
        .embedding_count = 1,
        .dimension_count = 2
    };
    struct embeddings second = {
        .embeddings = values,
        .embedding_count = 1,
        .dimension_count = 1
    };
    struct embeddings *input[] = {&first, &second};
    struct embeddings *output = NULL;
    assert(concat_embeddings(NULL, 1, &output) == 1);
    assert(concat_embeddings(input, 2, NULL) == 1);
    assert(concat_embeddings(input, 2, &output) == 1);
    input[1] = NULL;
    assert(concat_embeddings(input, 2, &output) == 1);
    first.embeddings = NULL;
    assert(concat_embeddings(input, 1, &output) == 1);
    first.embeddings = values;
    first.dimension_count = 0;
    assert(concat_embeddings(input, 1, &output) == 1);
    assert(output == NULL);
}


static void test_overflow(void)
{
    float value = 1;
    struct embeddings first = {
        .embeddings = &value,
        .embedding_count = SIZE_MAX,
        .dimension_count = 1
    };
    struct embeddings second = {
        .embeddings = &value,
        .embedding_count = 1,
        .dimension_count = 1
    };
    struct embeddings *input[] = {&first, &second};
    struct embeddings *output = NULL;
    assert(concat_embeddings(input, 2, &output) == 1);
    first.dimension_count = 2;
    assert(concat_embeddings(input, 1, &output) == 1);
    first.dimension_count = 1;
    first.embedding_count = SIZE_MAX / sizeof(float) + 1;
    assert(concat_embeddings(input, 1, &output) == 1);
    first.embedding_count = SIZE_MAX / sizeof(float);
    assert(concat_embeddings(input, 1, &output) == 1);
    assert(output == NULL);
}


int main(void)
{
    test_concat();
    test_single_and_repeated_input();
    test_empty();
    test_invalid_input();
    test_overflow();
    return 0;
}
