#include "smallrag/chunker.h"
#include "smallrag/embedder.h"

int map_text(char *text_path, char **text, size_t *text_length);

int load_embeddings(
        char *embeddings_path,
        struct chunks *chunks,
        struct embedder *embedder,
        struct embeddings *embeddings);
