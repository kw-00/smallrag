#ifndef SMALLRAG_RETRIEVAL_H
#define SMALLRAG_RETRIEVAL_H

#include <stddef.h>


/* Represents a single source text */
struct smallrag_text {
    char *text;
    size_t len;
};

/* Maps text to a file up to text's len. That mapping is read-only.
 */
int smallrag_mmap_text(int fd, struct smallrag_text *text);

/* Represents multiple text fragments.
 */
struct smallrag_text_frags {
    struct smallrag_text *frags;
    size_t count;
};

/* Splits text into fragments, returning the fragments via the frags parameter. Allocates memory for the fragments.
 * This means frags->frags should be freed when no longer useful. 
 *
 * Sets frags->count to match the number of fragments the text was split into.
 */
int smallrag_split_text(const struct smallrag_text *src, size_t frag_len, size_t frag_overlap, struct smallrag_text_frags *frags);


/* Represents vector embeddings for text fragments.
 */
struct smallrag_embds {
    float *vecs;
    size_t vec_dim;
    size_t vec_cnt;
    size_t tot_dim;
};

void smallrag_embds_init(struct smallrag_embds *embds, float *vecs, size_t vec_dim, size_t vec_cnt);

/* Maps embds to a file containing binary data, up to embds' tot_dim. That mapping is read-only.
 */
int smallrag_mmap_embds(int fd, struct smallrag_embds *embds);

/* Writes embeddings to a file. Compatible with smallrag_mmap_embds() in the sense that
 * such a file could then be read using the eforementioned function.
 */
int smallrag_write_embds(int fd, const struct smallrag_embds *embds);

struct smallrag_embd_provider;
/* Operations that can be performed on a certain provider type.
 */
struct smallrag_embd_provider_ops {
    int (*get_embds)(const struct smallrag_embd_provider *provider, const struct smallrag_text_frags *frags, struct smallrag_embds *embds);
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

/* Uses provider to get embeddings for frags.
 */
int smallrag_embd_provider_get_embds(const struct smallrag_embd_provider *provider, const struct smallrag_text_frags *frags, struct smallrag_embds *embds);

/* Releases resources associated with a provider.
 */
void smallrag_embd_provider_free(struct smallrag_embd_provider *provider);

#endif
