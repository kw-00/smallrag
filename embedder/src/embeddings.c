#include "smallrag/embeddings.h"

#include <stdlib.h>
#include <string.h>
#include "smallrag/allocation.h"
#include "smallrag/logging.h"


static void dispose_embeddings(struct embeddings *embeddings)
{
    free(embeddings->embeddings);
}


int concat_embeddings(
        struct embeddings **input,
        size_t input_count,
        struct embeddings *output)
{
    if (input_count == 0) {
        LOG_ERROR("input_count must be positive");
        return -1;
    }
    if (output == NULL || input == NULL) {
        LOG_ERROR("Invalid input embeddings");
        return -1;
    }

    size_t embedding_count = 0;
    size_t dimension_count = 0;
    for (size_t i = 0; i < input_count; i++) {
        if (input[i] == NULL) {
            LOG_ERROR("Invalid input embeddings");
            return -1;
        }
        if (input[i]->embedding_count == 0) {
            continue;
        }
        if (input[i]->embeddings == NULL || input[i]->dimension_count == 0) {
            LOG_ERROR("Invalid input embeddings");
            return -1;
        }
        if (dimension_count == 0) {
            dimension_count = input[i]->dimension_count;
        } else if (dimension_count != input[i]->dimension_count) {
            LOG_ERROR("Embedding dimensions must match");
            return -1;
        }
        if (__builtin_add_overflow(embedding_count, input[i]->embedding_count, &embedding_count)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
    }

    size_t total_vector_size;
    if (__builtin_mul_overflow(embedding_count, dimension_count, &total_vector_size)) {
        LOG_ERROR("Number overflow");
        return -1;
    }

    float *embedding_memory = NULL;
    if (total_vector_size != 0) {
        if (allocate_spans(1, &embedding_memory, sizeof(float), total_vector_size) == -1) {
            LOG_ERROR("Allocation failed");
            return -1;
        }
    }

    size_t vector_offset = 0;
    for (size_t i = 0; i < input_count; i++) {
        if (input[i]->embedding_count == 0) {
            continue;
        }
        size_t vector_size = input[i]->embedding_count * dimension_count;
        memcpy(
                embedding_memory + vector_offset,
                input[i]->embeddings,
                vector_size * sizeof(float));
        vector_offset += vector_size;
    }

    output->embeddings = embedding_memory;
    output->embedding_count = embedding_count;
    output->dimension_count = dimension_count;
    output->dispose = &dispose_embeddings;
    return 0;
}
