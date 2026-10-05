#include "smallrag/chunker_wrapper_functions.h"

void dispose_chunks(struct chunks *chunks)
{
    chunks->dispose(chunks);
}

int get_chunks(
        struct chunker *chunker, 
        char *text, 
        size_t text_length, 
        struct chunks *chunks)
{
    return chunker->get_chunks(chunker, text, text_length, chunks);
}

void dispose_chunker(struct chunker *chunker)
{
    chunker->dispose(chunker);
}

