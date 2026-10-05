/* Wrapper functions for using chunkers. Check chunker_core.h for more information.
 */

#ifndef SMALLRAG_CHUNKER_WRAPPER_FUNCTIONS_H
#define SMALLRAG_CHUNKER_WRAPPER_FUNCTIONS_H

#include <stddef.h>
#include <sys/types.h>
#include "smallrag/chunker_core.h"

/* Releases resources belonging to the chunks. Can be as simple as freeing memory used for the 
 * texts and sizes members.
 */
void dispose_chunks(struct chunks *chunks);


/* Uses a chunker to chunk text.
 *
 * chunker is the chunker being used.
 *
 * text/text_length are the source text and it's length, respectively.
 *
 * chunks is an out parameter.
 *
 * Returns -1 on error, 0 otherwise.
 */
int get_chunks(
        struct chunker *chunker, 
        char *text, 
        size_t text_length, 
        struct chunks *chunks);


/* Releases resources associated with a chunker.
 */
void dispose_chunker(struct chunker *chunker);

#endif
