#include "smallrag/normalization.h"

#include "smallrag/logging.h"
#include <math.h>
#include <stddef.h>

int l2_normalize(float *vector, size_t vector_size)
{
    if (vector_size == 0) {
        LOG_ERROR("vector_size must be greater than 0");
        return -1;
    }
    double quotient = 0;
    for (size_t i = 0; i < vector_size; i++) {
        quotient += (double)vector[i] * vector[i];
    }
    quotient = sqrt(quotient);
    for (size_t i = 0; i < vector_size; i++) {
        vector[i] = vector[i] / quotient;
    }
    return 0;
}
