#include "smallrag/prefix_char_chunker.h"

#include <stdlib.h>
#include <string.h>
#include "smallrag/char_chunker.h"
#include "smallrag/logging.h"


struct prefix_char_chunker_data {
    struct chunker char_chunker;
    size_t prefix_length;
    char *prefix;
};


static void dispose_chunks(struct chunks *chunks)
{
    // The arrays and all chunk texts share one allocation.
    free(chunks->texts);
}


static int prefix_chunks(
        const struct chunks *source,
        const char *prefix,
        size_t prefix_length,
        struct chunks *chunks)
{
    if (source->count == 0) {
        *chunks = (struct chunks) { .dispose = &dispose_chunks };
        return 0;
    }

    size_t texts_size;
    size_t sizes_size;
    if (__builtin_mul_overflow(sizeof(char *), source->count, &texts_size) ||
            __builtin_mul_overflow(sizeof(size_t), source->count, &sizes_size)) {
        LOG_ERROR("Number overflow");
        return -1;
    }

    size_t alignment = _Alignof(size_t);
    size_t padding = (alignment - texts_size % alignment) % alignment;
    size_t sizes_offset;
    size_t alloc_size;
    if (__builtin_add_overflow(texts_size, padding, &sizes_offset) ||
            __builtin_add_overflow(sizes_offset, sizes_size, &alloc_size)) {
        LOG_ERROR("Number overflow");
        return -1;
    }
    size_t text_offset = alloc_size;
    for (size_t i = 0; i < source->count; i++) {
        size_t chunk_size;
        if (__builtin_add_overflow(prefix_length, source->sizes[i], &chunk_size) ||
                __builtin_add_overflow(alloc_size, chunk_size, &alloc_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
    }

    void *memory = malloc(alloc_size);
    if (memory == NULL) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    char **texts = memory;
    size_t *sizes = (size_t *)((char *)memory + sizes_offset);
    char *next_text = (char *)memory + text_offset;
    for (size_t i = 0; i < source->count; i++) {
        texts[i] = next_text;
        sizes[i] = prefix_length + source->sizes[i];
        memcpy(next_text, prefix, prefix_length);
        memcpy(next_text + prefix_length, source->texts[i], source->sizes[i]);
        next_text += sizes[i];
    }

    chunks->texts = texts;
    chunks->sizes = sizes;
    chunks->count = source->count;
    chunks->dispose = &dispose_chunks;
    return 0;
}


static int get_chunks(
        struct chunker *chunker,
        char *text,
        size_t text_length,
        struct chunks *chunks)
{
    struct prefix_char_chunker_data *data = chunker->data;
    struct chunks source;
    if (data->char_chunker.get_chunks(&data->char_chunker, text, text_length, &source) == -1) {
        return -1;
    }
    int status = prefix_chunks(&source, data->prefix, data->prefix_length, chunks);
    source.dispose(&source);
    return status;
}


static void dispose(struct chunker *chunker)
{
    struct prefix_char_chunker_data *data = chunker->data;
    data->char_chunker.dispose(&data->char_chunker);
    free(data->prefix);
    free(data);
}


int init_prefix_char_chunker(struct chunker *chunker, char separator, const char *prefix)
{
    if (prefix == NULL) {
        LOG_ERROR("Prefix is NULL");
        return -1;
    }

    struct prefix_char_chunker_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    data->prefix = strdup(prefix);
    if (data->prefix == NULL) {
        LOG_ERROR("Allocation failed");
        free(data);
        return -1;
    }
    data->prefix_length = strlen(data->prefix);
    if (init_char_chunker(&data->char_chunker, separator) == -1) {
        free(data->prefix);
        free(data);
        return -1;
    }

    chunker->get_chunks = &get_chunks;
    chunker->dispose = &dispose;
    chunker->data = data;
    return 0;
}
