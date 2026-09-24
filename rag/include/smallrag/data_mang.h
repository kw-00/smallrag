#ifndef SMALLRAG_EMBEDDING_H
#define SMALLRAG_EMBEDDING_H

#include <stddef.h>


/* Represents a single source text */
struct smallrag_text {
    char *text;
    size_t len;
};

/* Maps text to a file up to text's text_len. That mapping is read-only.
 */
int smallrag_mmap_text(int fd, struct smallrag_text *text);

/* Represents multiple text fragments.
 */
struct smallrag_text_frags {
    struct smallrag_text *frags;
    size_t count;
};

/* Splits text into fragments.
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

int smallrag_embds_init(struct smallrag_embds *embds, float *vecs, size_t vec_dim, size_t vec_cnt);

/* Maps embds to a file containing binary data, up to embds' tot_dim. That mapping is read-only.
 */
int smallrag_mmap_embds(int fd, struct smallrag_embds *embds);

/* Writes embeddings to a file. Compatible with smallrag_mmap_embds() in the sense that
 * such a file could then be read using the eforementioned function.
 */
int smallrag_write_embds(int fd, const struct smallrag_embds *embds);


#endif
