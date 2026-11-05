#include "smallrag/chunks.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct expected_chunk {
    const char *text;
    size_t size;
};

#define EXPECTED_CHUNK(text) { text, sizeof(text) - 1 }

static size_t input_dispose_count;


static void dispose_input(struct chunks *chunks)
{
    input_dispose_count++;
    for (size_t i = 0; i < chunks->count; i++) {
        free(chunks->texts[i]);
    }
    free(chunks->texts);
    free(chunks->sizes);
    free(chunks);
}


static struct chunks *make_input(const struct expected_chunk *expected, size_t count)
{
    struct chunks *input = malloc(sizeof(*input));
    assert(input != NULL);
    input->texts = malloc(count * sizeof(char *));
    input->sizes = malloc(count * sizeof(size_t));
    assert(input->texts != NULL && input->sizes != NULL);
    for (size_t i = 0; i < count; i++) {
        input->texts[i] = malloc(expected[i].size);
        assert(input->texts[i] != NULL);
        memcpy(input->texts[i], expected[i].text, expected[i].size);
        input->sizes[i] = expected[i].size;
    }
    input->count = count;
    input->dispose = &dispose_input;
    return input;
}


static void expect_chunks(
        struct chunks *chunks, const struct expected_chunk *expected, size_t count)
{
    assert(chunks->count == count);
    for (size_t i = 0; i < count; i++) {
        assert(chunks->sizes[i] == expected[i].size);
        assert(memcmp(chunks->texts[i], expected[i].text, expected[i].size) == 0);
        if (i + 1 < count) {
            assert(chunks->texts[i + 1] == chunks->texts[i] + chunks->sizes[i]);
        }
    }
}


static void test_concat(void)
{
    const struct expected_chunk first_values[] = {
        EXPECTED_CHUNK("A"), EXPECTED_CHUNK("B\0C")
    };
    const struct expected_chunk second_values[] = {
        EXPECTED_CHUNK("DDDD"), EXPECTED_CHUNK("E")
    };
    const struct expected_chunk expected[] = {
        EXPECTED_CHUNK("A"), EXPECTED_CHUNK("B\0C"),
        EXPECTED_CHUNK("DDDD"), EXPECTED_CHUNK("E")
    };
    struct chunks *first = make_input(first_values, 2);
    struct chunks *second = make_input(second_values, 2);
    struct chunks *input[] = {first, second};
    size_t previous_dispose_count = input_dispose_count;
    struct chunks output;
    assert(concat_chunks(input, 2, &output) == 0);
    assert(input[0] == first && input[1] == second);
    assert(input_dispose_count == previous_dispose_count);
    assert(output.texts != first->texts && output.texts != second->texts);
    assert(output.sizes != first->sizes && output.sizes != second->sizes);
    assert(output.texts[0] != first->texts[0] && output.texts[2] != second->texts[0]);
    assert((uintptr_t)output.sizes % _Alignof(size_t) == 0);
    expect_chunks(&output, expected, 4);

    first->texts[0][0] = 'X';
    assert(output.texts[0][0] == 'A');
    output.texts[3][0] = 'Z';
    assert(second->texts[1][0] == 'E');
    output.texts[3][0] = 'E';
    first->dispose(first);
    second->dispose(second);
    assert(input_dispose_count == previous_dispose_count + 2);
    expect_chunks(&output, expected, 4);
    output.dispose(&output);
}


static void test_single_and_repeated_input(void)
{
    const struct expected_chunk values[] = { EXPECTED_CHUNK("AB") };
    const struct expected_chunk expected[] = { EXPECTED_CHUNK("AB"), EXPECTED_CHUNK("AB") };
    struct chunks *source = make_input(values, 1);
    struct chunks *input[] = {source};
    struct chunks first;
    assert(concat_chunks(input, 1, &first) == 0);
    struct chunks *repeated[] = {&first, &first};
    struct chunks second;
    assert(concat_chunks(repeated, 2, &second) == 0);
    first.dispose(&first);
    assert(memcmp(source->texts[0], "AB", 2) == 0);
    source->dispose(source);
    expect_chunks(&second, expected, 2);
    second.dispose(&second);
}


static void test_empty(void)
{
    struct chunks output;
    assert(concat_chunks(NULL, 0, &output) == 0);
    assert(output.count == 0 && output.texts == NULL && output.sizes == NULL);
    output.dispose(&output);

    struct chunks empty = {0};
    struct chunks *empty_input[] = {&empty};
    assert(concat_chunks(empty_input, 1, &output) == 0);
    assert(output.count == 0);
    output.dispose(&output);

    char *texts[] = {NULL};
    size_t sizes[] = {0};
    struct chunks source = { .texts = texts, .sizes = sizes, .count = 1 };
    struct chunks *input[] = {&empty, &source, &empty};
    assert(concat_chunks(input, 3, &output) == 0);
    assert(output.count == 1 && output.sizes[0] == 0);
    output.dispose(&output);
}


static void test_invalid_input(void)
{
    char text = 'A';
    char *texts[] = {&text};
    size_t sizes[] = {1};
    struct chunks source = { .texts = texts, .sizes = sizes, .count = 1 };
    struct chunks *input[] = {&source, NULL};
    struct chunks output = {0};
    assert(concat_chunks(NULL, 1, &output) == 1);
    assert(concat_chunks(input, 1, NULL) == 1);
    assert(concat_chunks(input, 2, &output) == 1);
    source.texts = NULL;
    assert(concat_chunks(input, 1, &output) == 1);
    source.texts = texts;
    source.sizes = NULL;
    assert(concat_chunks(input, 1, &output) == 1);
    source.sizes = sizes;
    texts[0] = NULL;
    assert(concat_chunks(input, 1, &output) == 1);
    assert(output.dispose == NULL);
}


static void test_overflow(void)
{
    char text = 'A';
    char *texts[] = {&text, &text};
    size_t sizes[] = {1, 1};
    struct chunks first = { .texts = texts, .sizes = sizes, .count = SIZE_MAX };
    struct chunks second = { .texts = texts, .sizes = sizes, .count = 1 };
    struct chunks *input[] = {&first, &second};
    struct chunks output = {0};
    assert(concat_chunks(input, 2, &output) == 1);
    assert(concat_chunks(input, 1, &output) == 1);
    first.count = 2;
    sizes[0] = SIZE_MAX;
    assert(concat_chunks(input, 1, &output) == 1);
    first.count = 1;
    assert(concat_chunks(input, 1, &output) == 1);
    assert(output.dispose == NULL);
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
