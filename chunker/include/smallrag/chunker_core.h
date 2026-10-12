/* Structs for polymorphic text chunking.
 */

#ifndef SMALLRAG_CHUNKER_CORE_H
#define SMALLRAG_CHUNKER_CORE_H

#include <stddef.h>
#include <sys/types.h>

/* Represents a collection of text chunks. texts[i] is the text of the i-th chunk and sizes[i] is
 * the length of that text.
 *
 * Running an instance's dispose() method with that instance as its parameter must release
 * resources owned by that instance.
 */
struct chunks {
    char **texts;
    size_t *sizes;
    size_t count;
    void (*dispose)(struct chunks *chunks);
};


/* Struct used for polymorphic text chunking.
 *
 * Has two methods. Always call a chunker instance's methods with that instance as its first 
 * parameter.
 *
 * The get_chunks() method can be used to split text into chunks. 
 *
 * The text parameter is the text being split into chunks.
 *
 * text_length denotes that text's length. It must be greater than 0.
 *
 * chunks is an out parameter.
 *
 * It must not modify text, though the chunks out parameter may be a mutable view over text.
 *
 * 0 is returned on success and -1 on error. The out parameter is undefined after error.
 *
 * The dispose() method can be used to release resources associated with a chunker.
 */
struct chunker {
    int (*get_chunks)(
            struct chunker *chunker, 
            char *text, 
            size_t text_length, 
            struct chunks *chunks);
    void (*dispose)(struct chunker *chunker);
    void *data;
};

#endif
