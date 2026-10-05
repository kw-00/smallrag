#ifndef SMALLRAG_CHUNKER_CORE_H
#define SMALLRAG_CHUNKER_CORE_H

#include <stddef.h>
#include <sys/types.h>

/* Represents a collection of text chunks. texts[i] is the text of the i-th chunk and sizes[i] is
 * the length of that text.
 *
 * Running the dispose() method on texts releases resources owned by the struct. This may 
 * be as simple as freeing memory used for texts and sizes.
 */
struct chunks {
    char **texts;
    size_t *sizes;
    size_t count;
    void (*dispose)(struct chunks *chunks);
};


/* Polymorphic struct used by get_chunks() for splitting text into chunks. Use get_chunks() to
 * split text into chunks, and dispose_chunker() to dispose of the chunker after its use.
 *
 * The goal of get_chunks()/struct chunker is to enable polymorphic use of various implementations
 * of text chunking.
 *
 * An implementation should contain a function that initializes struct chunker in a way that then
 * allows chunking text by calling get_chunks() with the chunker as a parameter.
 *
 * It should give the chunker a working get_chunks() method and a dispose() method, which can be
 * a no-op.
 *
 * The data member is there to allow a chunker to carry data that can then be used by its
 * get_chunks() method.
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
