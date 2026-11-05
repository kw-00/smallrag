#include "smallrag/chunks.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "smallrag/allocation.h"
#include "smallrag/logging.h"


static void dispose_chunks(struct chunks *chunks)
{
    free(chunks->texts);
}


int concat_chunks(
        struct chunks **input,
        size_t input_count,
        struct chunks *output)
{
    if (output == NULL || (input_count != 0 && input == NULL)) {
        LOG_ERROR("Invalid input chunks");
        return 1;
    }

    size_t chunk_count = 0;
    for (size_t i = 0; i < input_count; i++) {
        if (input[i] == NULL || (input[i]->count != 0
                    && (input[i]->texts == NULL || input[i]->sizes == NULL))) {
            LOG_ERROR("Invalid input chunks");
            return 1;
        }
        if (__builtin_add_overflow(chunk_count, input[i]->count, &chunk_count)) {
            LOG_ERROR("Number overflow");
            return 1;
        }
    }
    if (chunk_count > SIZE_MAX / (sizeof(char *) + sizeof(size_t))) {
        LOG_ERROR("Number overflow");
        return 1;
    }
    if (chunk_count == 0) {
        *output = (struct chunks) { .dispose = &dispose_chunks };
        return 0;
    }

    size_t text_storage_size = 0;
    for (size_t i = 0; i < input_count; i++) {
        for (size_t j = 0; j < input[i]->count; j++) {
            if (input[i]->sizes[j] != 0 && input[i]->texts[j] == NULL) {
                LOG_ERROR("Invalid input chunks");
                return 1;
            }
            if (__builtin_add_overflow(
                        text_storage_size, input[i]->sizes[j], &text_storage_size)) {
                LOG_ERROR("Number overflow");
                return 1;
            }
        }
    }

    char **texts;
    size_t *sizes;
    char *text_storage;
    if (allocate_spans(
                3,
                &texts,
                sizeof(char *),
                chunk_count,
                &sizes,
                sizeof(size_t),
                chunk_count,
                _Alignof(size_t),
                &text_storage,
                sizeof(char),
                text_storage_size,
                _Alignof(char)) == -1) {
        LOG_ERROR("Allocation failed");
        return 1;
    }

    size_t chunk_index = 0;
    char *next_text = text_storage;
    for (size_t i = 0; i < input_count; i++) {
        for (size_t j = 0; j < input[i]->count; j++) {
            texts[chunk_index] = next_text;
            sizes[chunk_index] = input[i]->sizes[j];
            if (sizes[chunk_index] != 0) {
                memcpy(next_text, input[i]->texts[j], sizes[chunk_index]);
            }
            next_text += sizes[chunk_index];
            chunk_index++;
        }
    }

    output->texts = texts;
    output->sizes = sizes;
    output->count = chunk_count;
    output->dispose = &dispose_chunks;
    return 0;
}
