#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "smallrag/char_chunker.h"
#include "smallrag/chunker.h"
#include "smallrag/prefix_char_chunker.h"


struct expected_chunk {
    const char *text;
    size_t size;
};

#define EXPECTED_CHUNK(text) { text, sizeof(text) - 1 }


static void expect_chunks(
        struct chunks *chunks,
        const struct expected_chunk *expected_chunks,
        size_t expected_chunk_count)
{
    assert(chunks->count == expected_chunk_count);
    for (size_t i = 0; i < chunks->count; i++) {
        assert(chunks->sizes[i] == expected_chunks[i].size);
        assert(memcmp(chunks->texts[i], expected_chunks[i].text, chunks->sizes[i]) == 0);
    }
}


static void expect_split(
        struct chunker *chunker,
        char *text,
        size_t text_length,
        const struct expected_chunk *expected_chunks,
        size_t expected_chunk_count)
{
    struct chunks chunks;
    assert(get_chunks(chunker, text, text_length, &chunks) == 0);
    expect_chunks(&chunks, expected_chunks, expected_chunk_count);
    dispose_chunks(&chunks);
}


static void test_split(void)
{
    struct chunker chunker;
    assert(init_prefix_char_chunker(&chunker, '|', "prefix: ") == 0);
    char text[] = "A|BB|CCC";
    const struct expected_chunk expected[] = {
        EXPECTED_CHUNK("prefix: A"),
        EXPECTED_CHUNK("prefix: BB"),
        EXPECTED_CHUNK("prefix: CCC"),
    };
    expect_split(&chunker, text, sizeof(text) - 1, expected, 3);
    assert(memcmp(text, "A|BB|CCC", sizeof(text)) == 0);
    dispose_chunker(&chunker);
}


static void test_separators(void)
{
    struct chunker chunker;
    assert(init_prefix_char_chunker(&chunker, '|', "| ") == 0);
    char text[] = "||A|||BB||";
    const struct expected_chunk expected[] = {
        EXPECTED_CHUNK("| A"),
        EXPECTED_CHUNK("| BB"),
    };
    expect_split(&chunker, text, sizeof(text) - 1, expected, 2);

    char no_separator[] = "single";
    const struct expected_chunk single[] = { EXPECTED_CHUNK("| single") };
    expect_split(&chunker, no_separator, sizeof(no_separator) - 1, single, 1);

    char only_separators[] = "||||";
    expect_split(&chunker, only_separators, sizeof(only_separators) - 1, NULL, 0);
    dispose_chunker(&chunker);
}


static void test_empty_prefix(void)
{
    struct chunker chunker;
    assert(init_prefix_char_chunker(&chunker, '|', "") == 0);
    char text[] = "A|BB";
    struct chunks chunks;
    assert(get_chunks(&chunker, text, sizeof(text) - 1, &chunks) == 0);
    text[0] = 'X';
    const struct expected_chunk expected[] = {
        EXPECTED_CHUNK("A"),
        EXPECTED_CHUNK("BB"),
    };
    expect_chunks(&chunks, expected, 2);
    dispose_chunks(&chunks);
    dispose_chunker(&chunker);
}


static void test_binary_text(void)
{
    struct chunker chunker;
    assert(init_prefix_char_chunker(&chunker, '\0', "P: ") == 0);
    char nul_separated[] = { '\0', 'A', '\0', '\0', 'B', 'B', '\0' };
    const struct expected_chunk separated[] = {
        EXPECTED_CHUNK("P: A"),
        EXPECTED_CHUNK("P: BB"),
    };
    expect_split(&chunker, nul_separated, sizeof(nul_separated), separated, 2);
    dispose_chunker(&chunker);

    assert(init_prefix_char_chunker(&chunker, '|', "P: ") == 0);
    char embedded_nul[] = { 'A', '\0', 'B', '|', 'C' };
    const struct expected_chunk embedded[] = {
        EXPECTED_CHUNK("P: A\0B"),
        EXPECTED_CHUNK("P: C"),
    };
    expect_split(&chunker, embedded_nul, sizeof(embedded_nul), embedded, 2);
    dispose_chunker(&chunker);
}


static void test_owned_lifetimes(void)
{
    struct chunker chunker;
    char prefix[] = "old ";
    assert(init_prefix_char_chunker(&chunker, '|', prefix) == 0);
    prefix[0] = 'X';

    char *text = malloc(sizeof("A|BB"));
    assert(text != NULL);
    memcpy(text, "A|BB", sizeof("A|BB"));
    struct chunks first;
    assert(get_chunks(&chunker, text, sizeof("A|BB") - 1, &first) == 0);
    assert(memcmp(text, "A|BB", sizeof("A|BB")) == 0);

    char other[] = "C|DD";
    struct chunks second;
    assert(get_chunks(&chunker, other, sizeof(other) - 1, &second) == 0);
    free(text);
    dispose_chunker(&chunker);

    const struct expected_chunk first_expected[] = {
        EXPECTED_CHUNK("old A"),
        EXPECTED_CHUNK("old BB"),
    };
    expect_chunks(&first, first_expected, 2);
    dispose_chunks(&first);

    const struct expected_chunk second_expected[] = {
        EXPECTED_CHUNK("old C"),
        EXPECTED_CHUNK("old DD"),
    };
    expect_chunks(&second, second_expected, 2);
    dispose_chunks(&second);
}


int main(void)
{
    test_split();
    test_separators();
    test_empty_prefix();
    test_binary_text();
    test_owned_lifetimes();
    return 0;
}
