#ifndef SMALLRAG_CHUNKS_H
#define SMALLRAG_CHUNKS_H

#include "smallrag/chunker_core.h"

/* Copies and concatenates the chunks pointed to by input into output. input_count is the number
 * of input structs.
 *
 * Does not take ownership of input or its resources.
 *
 * Returns 0 on success, 1 on failure.
 */
int concat_chunks(
        struct chunks **input,
        size_t input_count,
        struct chunks *output);

#endif
