#include "smallrag/data_mang.h"

int smallrag_mmap_text(int fd, struct smallrag_text *text)
{
    void *mapping = mmap(NULL, text->len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) throw();
    text->text = mapping;
}

int smallrag_split_text(const struct smallrag_text *src, size_t frag_len, size_t frag_overlap, struct smallrag_text_frags *frags)
{
    size_t cnt = 0;
    size_t offset = 0;
    while (true) {
        offset += frag_len;
        if (offset >= src->len) break;
        offset -= frag_overlap;
        cnt++;
    }
    if (cnt == 0) {
        return -1;
    }
    struct smallrag_text *frag_span = malloc(sizeof(struct smallrag_text) * cnt);
    size_t last_frag_len = src->len % frag_len;
    if (last_frag_len == 0) last_frag_len = frag_len;
    for (int i = 0; i < cnt - 1; i++) {



int smallrag_embds_init(struct smallrag_embds *embds, float *vecs, size_t vec_dim, size_t vec_cnt)
{
    embds->vec = vec;
    embds->vec_dim = vec_dim;
    embds->vec_cnt = vec_cnt;
    embds->tot_dim = vec_dim * vec_cnt;
}

int smallrag_mmap_embds(int fd, struct smallrag_embds *embds);

int smallrag_write_embds(int fd, struct smallrag_embds *embds);



