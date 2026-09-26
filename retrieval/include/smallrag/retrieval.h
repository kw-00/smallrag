#ifndef SMALLRAG_RETRIEVAL_H
#define SMALLRAG_RETRIEVAL_H

#include <stddef.h>


/* Maps the string passed through the text parameter to file fd, up the the entire file's length.
 *
 * Returns file length on success, -1 on failure.
 */
ssize_t smallrag_mmap_text(int fd, char **text);

/* Splits text into fragments using a specific delimiter character.
 *
 * source_represents the sourc text, with src_len denoting its length. delim is the character used 
 * to split the text.
 *
 * The function returns -1 on failure and number of resulting text fragments on success.
 *
 * Furthermore, there are two out parameters. (*fragm_sizes) is a span of numbers, each element 
 * representing the size of a single fragment. (*fragments) is a span of character spans, each  
 * character span representing the text of a single fragment.
 * (*fragm_sizes)[i] corresponds to (*fragments)[i].
 *
 * Allocates memory for fragm_sizes and fragments, but not for the text itself — the text of each
 * fragment is merely a view over the source text.
 *
 * Therfore, to free memory allocated by this function, call free(*fragm_sizes) and free(*fragments),
 * but not free((*fragments)[i]).
 */
ssize_t smallrag_split_text(
        const char *source, size_t src_len, char delim, size_t **fragm_sizes, char ***fragments);


/* Represents vector embeddings for text fragments.
 */
struct smallrag_embds {
    float *vecs;
    size_t dim;
    size_t cnt;
};

/* Maps embds to a file containing binary data, up to embds' dim * cnt. That mapping is 
 * read-only.
 *
 * Returns total embds size (embds->dim * embds->cnt) on success, -1 on failure.
 */
ssize_t smallrag_mmap_embds(int fd, struct smallrag_embds *embds);

/* Writes embeddings to a file. Compatible with smallrag_mmap_embds() in the sense that
 * such a file could then be read using the eforementioned function.
 *
 * Returns number of bytes written on success, -1 on failure.
 */
ssize_t smallrag_write_embds(int fd, const struct smallrag_embds *embds);

struct smallrag_embd_provider;
/* Operations that can be performed on a certain provider type.
 */
struct smallrag_embd_provider_ops {
    int (*get_embds)(
            const struct smallrag_embd_provider *provider, 
            size_t fragm_cnt, 
            const size_t *fragm_sizes, 
            const char **fragments, 
            struct smallrag_embds *embds);
    void (*free)(struct smallrag_embd_provider *provider);
};


/* Represents an embedding provider of unspecified types for polymorphic behaviour.
 * ops represent how operations of the provider are implemented; provider_data is a type-erased representation of the provider
 * which ops should be aware of.
 */
struct smallrag_embd_provider {
    struct smallrag_embd_provider_ops *ops;
    void *provider_data;
};

/* Uses provider to get vector embeddings for text fragments.
 *
 * fragm_cnt represents the number of text fragments.
 * fragm_sizes span of numbers, each element representing the size of a text fragment.
 * fragments is a span of spans of strings. Each span of strings represents the contents of
 * a single text fragment.
 */
int *smallrag_get_embds(
        const struct smallrag_embd_provider *provider, 
        size_t fragm_cnt, 
        const size_t *fragm_sizes, 
        const char **fragments, 
        struct smallrag_embds *embds);

/* Releases resources associated with a provider.
 */
void smallrag_embd_provider_free(struct smallrag_embd_provider *provider);

#endif
