#include "smallrag/dot_product.h"

#include <stddef.h>

double dot_product(float *vector_x, float *vector_y, size_t vector_size)
{
    double dot = 0;
    for (size_t i = 0; i < vector_size; i++) {
        dot += (double)vector_x[i] * vector_y[i];
    }
    return dot;
}
