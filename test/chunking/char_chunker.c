#include <string.h>
#include <assert.h>
#include "smallrag/char_chunker.h"
#include "smallrag/chunker.h"

#include <stdio.h> 
static void expect_split(
        struct chunker *chunker, 
        char *text, 
        char **expected_chunks,
        size_t expected_chunk_count)
{
    const size_t text_length = strlen(text);
    struct chunks chunks;
    assert(get_chunks(chunker, text, text_length, &chunks) == 0);
    assert(chunks.count == expected_chunk_count);
    for (size_t i = 0; i < chunks.count; i++) {
        size_t expected_chunk_size = strlen(expected_chunks[i]);
        assert(chunks.sizes[i] == expected_chunk_size);
        assert(memcmp(chunks.texts[i], expected_chunks[i], chunks.sizes[i]) == 0);
    }
    dispose_chunks(&chunks);
}

void main(void)
{
    struct chunker chunker;
    init_char_chunker(&chunker, '|');
    char *expected_chunks[3];
    expected_chunks[0] = "A";
    expected_chunks[1] = "BB";
    expected_chunks[2] = "CCC";
    expect_split(&chunker, "A|BB|CCC", expected_chunks, 3);
}
