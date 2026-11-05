#include "smallrag/embeddings.h"

#include <stdlib.h>
#include <string.h>
#include "smallrag/allocation.h"
#include "smallrag/logging.h"


static void dispose_embeddings(struct embeddings *embeddings)
{
    free(embeddings);
}


int concat_embeddings(
        struct embeddings **input,
        size_t input_count,
        struct embeddings **output)
{
    if (output == NULL || (input_count != 0 && input == NULL)) {
        LOG_ERROR("Invalid input embeddings");
        return 1;
    }

    size_t embedding_count = 0;
    size_t dimension_count = 0;
    for (size_t i = 0; i < input_count; i++) {
        if (input[i] == NULL) {
            LOG_ERROR("Invalid input embeddings");
            return 1;
        }
        if (input[i]->embedding_count == 0) {
            continue;
        }
        if (input[i]->embeddings == NULL || input[i]->dimension_count == 0) {
            LOG_ERROR("Invalid input embeddings");
            return 1;
        }
        if (dimension_count == 0) {
            dimension_count = input[i]->dimension_count;
        } else if (dimension_count != input[i]->dimension_count) {
            LOG_ERROR("Embedding dimensions must match");
            return 1;
        }
        if (__builtin_add_overflow(embedding_count, input[i]->embedding_count, &embedding_count)) {
            LOG_ERROR("Number overflow");
            return 1;
        }
    }

    size_t total_vector_size;
    if (__builtin_mul_overflow(embedding_count, dimension_count, &total_vector_size)) {
        LOG_ERROR("Number overflow");
        return 1;
    }

    struct embeddings *result;
    float *embedding_memory;
    if (allocate_spans(
                2,
                &result,
                sizeof(*result),
                1,
                &embedding_memory,
                sizeof(float),
                total_vector_size,
                _Alignof(float)) == -1) {
        LOG_ERROR("Allocation failed");
        return 1;
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

    result->embeddings = total_vector_size == 0 ? NULL : embedding_memory;
    result->embedding_count = embedding_count;
    result->dimension_count = dimension_count;
    result->dispose = &dispose_embeddings;
    *output = result;
    return 0;
}
