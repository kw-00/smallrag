
#ifndef SMALLRAG_PREFIX_CHAR_CHUNKER_H
#define SMALLRAG_PREFIX_CHAR_CHUNKER_H

#include "smallrag/chunker_core.h"

/* Initializes a chunker that splits text using separator and adds a prefix to each chunk.
 *
 * prefix is copied — the function does not take ownership of it.
 *
 * Returns 0 on success, -1 on error.
 */
int init_prefix_char_chunker(struct chunker *chunker, char separator, const char *prefix);

#endif
