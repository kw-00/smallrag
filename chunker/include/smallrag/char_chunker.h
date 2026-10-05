#ifndef SMALLRAG_CHAR_CHUNKER_H
#define SMALLRAG_CHAR_CHUNKER_H

#include "smallrag/chunker_core.h"

/* Initializes a chunker that splits text using separator.
 *
 * Returns 0 on success, -1 on error.
 */
int init_char_chunker(struct chunker *chunker, char separator);

#endif
