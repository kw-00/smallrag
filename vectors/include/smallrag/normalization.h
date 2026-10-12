#ifndef SMALLRAG_NORMALIZATION_H
#define SMALLRAG_NORMALIZATION_H

#include <stddef.h>


/* Applies inplace L2 normalization to vector.
 */
int l2_normalize(float *vector, size_t vector_size);

#endif
