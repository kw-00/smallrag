#ifndef SMALLRAG_DATA_LOADING_H
#define SMALLRAG_DATA_LOADING_H

#include "smallrag/chunker.h"
#include "smallrag/embedder.h"

/* Maps a file with path of text_path using mmap.
 *
 * text and text_length are out parameters. *text ends up being mapped to the file's beginning.
 * *text_length ends up as the size of the file in bytes.
 *
 * Returns 0 on success and -1 on failure.
 *
 * On failure, text and text_length are undefined.
 */
int map_text(char *text_path, char **text, size_t *text_length);

/* Attempts to load vector embeddings for chunks.
 *
 * embeddings_path is the path for the embeddings. If it already exists and is not empty,
 * embeddings are loaded from it.
 *
 * Otherwise, a file is created under this path and embedder is used to get vector embeddings
 * from chunks.
 *
 * embeddings is an out parameter that contains the resulting embeddings.
 *
 * 0 is returned on success, -1 on failure. On failure, the value of embeddings is undefined.
 */
int load_embeddings(
        char *embeddings_path,
        struct chunks *chunks,
        struct embedder *embedder,
        struct embeddings *embeddings);

#endif
