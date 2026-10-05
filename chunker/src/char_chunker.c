#include "smallrag/char_chunker.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include "smallrag/logging.h"


static void dispose_chunks(struct chunks *chunks) 
{
    free(chunks->texts);
}

static int split_text(
        char *text, size_t text_length, char separator, struct chunks *chunks)
{
    size_t chunk_count = 0;
    {
        bool in_chunk = false;
        for (size_t i = 0; i < text_length; i++) {
            if (text[i] == separator) {
                if (in_chunk) {
                    chunk_count++;
                }
                in_chunk = false;
            } else {
                in_chunk = true;
            }
        }
        if (in_chunk) {
            chunk_count++;
        }
    }
    char **texts;
    size_t *sizes;
    {
        size_t alloc_size;
        if (__builtin_mul_overflow(sizeof(char *) + sizeof(size_t), chunk_count, &alloc_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
        void *chunk_memory = malloc(alloc_size);
        if (chunk_memory == NULL) {
            LOG_ERROR("Allocation failed");
            return -1;
        }
        texts = chunk_memory;
        sizes = (size_t *)(texts + chunk_count);
    }

    {
        size_t chunk_idx = 0;
        bool in_chunk = false;
        for (size_t i = 0; i < text_length; i++) {
            if (text[i] == separator) {
                if (in_chunk) {
                    chunk_idx++;
                    in_chunk = false;
                } 
            } else {
                if (!in_chunk) {
                    texts[chunk_idx] = text + i;
                    sizes[chunk_idx] = 1;
                    in_chunk = true;
                } else {
                    sizes[chunk_idx]++;
                }
            }
        }
    }
    chunks->texts = texts;
    chunks->sizes = sizes;
    chunks->count = chunk_count;
    chunks->dispose = &dispose_chunks;
    return 0;
}


static int get_chunks(
        struct chunker *chunker,
        char *text, 
        size_t text_length, 
        struct chunks *chunks)
{
    char separator = (char)(uintptr_t)chunker->data;
    return split_text(text, text_length, separator, chunks);
}


static void dispose(struct chunker *chunker) 
{
    // No op
}


int init_char_chunker(struct chunker *chunker, char separator)
{
    chunker->get_chunks = &get_chunks;
    chunker->dispose = &dispose;
    chunker->data = (void *)(uintptr_t)separator;
    return 0;
}

