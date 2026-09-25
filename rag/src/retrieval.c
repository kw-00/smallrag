#include "smallrag/retrieval.h"
#include "smallrag/err.h"
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>

int smallrag_mmap_text(int fd, struct smallrag_text *text)
{
    void *mapping = mmap(NULL, text->len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) {
        smallrag_pusherrstd();
        return -1;
    }
    text->text = mapping;
    return 0;
}

int smallrag_split_text(const struct smallrag_text *src, size_t frag_len, size_t frag_overlap, struct smallrag_text_frags *frags)
{
    if (frag_overlap >= frag_len) {
        smallrag_pusherrmsg(smallragERR_ARGUMENT, "frag_overlap must be smaller than frag_len");
        return -1;
    }
    if (frag_len < 1) {
        smallrag_pusherrmsg(smallragERR_ARGUMENT, "frag_len must be greater than 0");
        return -1;
    }
    if (src->len == 0) {
        return 0;
    }
    size_t cnt = 1;
    struct smallrag_text *frag_span;
    {
        size_t offset = 0;
        while (true) {
            offset += frag_len;
            if (offset >= src->len) break;
            offset -= frag_overlap;
            cnt++;
        }
        frag_span = malloc(sizeof(struct smallrag_text) * cnt);
        if (frag_span == NULL) {
            smallrag_pusherrstd();
            return -1;
        }
    }

    size_t last_frag_len = src->len % frag_len;
    if (last_frag_len == 0) last_frag_len = frag_len;
    size_t offset = 0;
    size_t offset_delta = frag_len - frag_overlap;

    for (int i = 0; i < cnt - 1; i++) {
        frag_span[i].text = src->text + offset;
        frag_span[i].len = frag_len;
        offset += offset_delta;
    }
    frag_span[cnt - 1].text = src->text + offset;
    frag_span[cnt - 1].len = last_frag_len;
    frags->frags = frag_span;
    frags->count = cnt;
    return 0;
}


void smallrag_embds_init(struct smallrag_embds *embds, float *vecs, size_t vec_dim, size_t vec_cnt)
{
    embds->vecs = vecs;
    embds->vec_dim = vec_dim;
    embds->vec_cnt = vec_cnt;
    embds->tot_dim = vec_dim * vec_cnt;
}

int smallrag_mmap_embds(int fd, struct smallrag_embds *embds)
{
    void *mapping = mmap(NULL, embds->tot_dim * sizeof(float), PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) {
        smallrag_pusherrstd();
        return -1;
    }
    embds->vecs = mapping;
    return 0;
}

int smallrag_write_embds(int fd, const struct smallrag_embds *embds)
{
    if (ftruncate(fd, 0) == -1) {
        smallrag_pusherrstd();
        return -1;
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
        smallrag_pusherrstd();
        return -1;
    }
    void *src = embds->vecs;
    size_t tot_bytes = embds->tot_dim * sizeof(float);
    size_t tot_written = 0;
    while (tot_written < tot_bytes) {
        ssize_t written = write(fd, src + tot_written, tot_bytes - tot_written);
        if (written == -1) {
            smallrag_pusherrstd();
            return -1;
        }
        tot_written += written;
    }
    return 0;
}


int smallrag_embd_model_get_embds(const struct smallrag_embd_model *model, const struct smallrag_text_frags *frags, struct smallrag_embds *embds) 
{
    return model->ops->get_embds(model, frags, embds);
}

int smallrag_embd_model_free(struct smallrag_embd_model *model) 
{
    return model->ops->free(model);
}


