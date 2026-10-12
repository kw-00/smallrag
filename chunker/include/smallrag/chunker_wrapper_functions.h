/* Wrapper functions for using chunkers. Check chunker_core.h for more information about chunkers
 * themselves.
 */

#ifndef SMALLRAG_CHUNKER_WRAPPER_FUNCTIONS_H
#define SMALLRAG_CHUNKER_WRAPPER_FUNCTIONS_H

#include <stddef.h>
#include <sys/types.h>
#include "smallrag/chunker_core.h"

/* Function wrapper for chunks' dispose() method.
 */
void dispose_chunks(struct chunks *chunks);

/* Function wrapper for a chunker's get_chunks() method.
 */
int get_chunks(
        struct chunker *chunker, 
        char *text, 
        size_t text_length, 
        struct chunks *chunks);


/* Function wrapper for a chunker's dispose() method.
 */
void dispose_chunker(struct chunker *chunker);

#endif
