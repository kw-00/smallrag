#ifndef SMALLRAG_EMBEDDING_H
#define SMALLRAG_EMBEDDING_H

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

struct smallrag_embd_model;
/* Operations that can be performed on a certain model type.
 */
struct smallrag_embd_model_ops {
    int (*get_embds)(const struct smallrag_embd_model *model, const struct smallrag_text_frags *frags, struct smallrag_embds *embds);
    int (*free)(struct smallrag_embd_model *model);
};


/* Represents an embedding model of unspecified types for polymorphic behaviour.
 * ops represent how operations of the model are implemented; model_data is a type-erased representation of the model
 * which ops should be aware of.
 */
struct smallrag_embd_model {
    struct smallrag_embd_model_ops *ops;
    void *model_data;
};

/* Uses model to get embeddings for frags.
 */
int smallrag_embd_model_get_embds(const struct smallrag_embd_model *model, const struct smallrag_text_frags *frags, struct smallrag_embds *embds);

/* Releases resources associated with a model.
 */
int smallrag_embd_model_free(struct smallrag_embd_model *model);

#endif
