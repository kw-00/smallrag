#include "smallrag/retrieval.h"
#include "smallrag/err.h"
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>


ssize_t smallrag_mmap_text(int fd, char **text)
{
    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        smallrag_pusherrstd();
        return -1;
    }
    const off_t fsize = statbuf.st_size;
    if (fsize > SSIZE_MAX) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    const ssize_t size = fsize;
    void *mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        smallrag_pusherrstd();
        return -1;
    }
    *text = mapped;
    return size;
}

ssize_t smallrag_split_text(
        char *source, 
        size_t src_len, 
        char delim, 
        size_t **fragm_sizes, char ***fragments)
{
    size_t cnt = 0;
    bool in_fragm = false;
    for (int i = 0; i < src_len; i++) {
        if (source[i] != delim) {
            if (!in_fragm) {
                cnt++;
                in_fragm = true;
            }
        } else {
            in_fragm = false;
        }
    }

    size_t sizes_alloc_size;
    if (__builtin_mul_overflow(sizeof(size_t), cnt, &sizes_alloc_size)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    size_t fragments_alloc_size;
    if (__builtin_mul_overflow(sizeof(char *), cnt, &fragments_alloc_size)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    size_t alloc_size;
    if (__builtin_add_overflow(sizes_alloc_size, fragments_alloc_size, &alloc_size)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }

    void *full_alloc = malloc(alloc_size);
    if (full_alloc == NULL) {
        smallrag_pusherrstd();
        return -1;
    }
    *fragm_sizes = full_alloc;

    *fragments = full_alloc + sizes_alloc_size; 
    
    in_fragm = false;
    size_t fragm_idx = 0;
    size_t fragm_size = 1;
    size_t i = 0;
    while(true) {
        if (i < src_len) {
            if (source[i] != delim) {
                if (!in_fragm) {
                    *fragments[fragm_idx] = source + i;
                    in_fragm = true;
                } else {
                    // Never overflows, as its size is bounded up to src_len through i
                    fragm_size++;
                }
            } else {
                if (in_fragm) {
                    *fragm_sizes[fragm_idx] = fragm_size; 
                    in_fragm = false;
                    fragm_size = 1;
                }
            }
        } else if (in_fragm) {
            *fragm_sizes[fragm_idx] = fragm_size; 
            in_fragm = false;
        } else {
            break;
        }
        i++;
    }
    if (cnt > SSIZE_MAX) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    return cnt;
}


ssize_t smallrag_mmap_embds(int fd, struct smallrag_embds *embds)
{
    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        smallrag_pusherrstd();
        return -1;
    }
    const off_t fsize = statbuf.st_size;
    if (fsize > SIZE_MAX) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    ssize_t vec_size;
    if (__builtin_mul_overflow(embds->dim, embds->cnt, &vec_size)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    size_t vec_bytes;
    if (__builtin_mul_overflow(vec_size, sizeof(float), &vec_bytes)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }

    if (vec_bytes != fsize) {
        smallrag_pusherrmsg(
                smallragERR_INVALID_STATE, 
                "Total embds size (dimensions * embedding count) does not match file size");
        return -1;
    }
    void *mapping = mmap(NULL, vec_bytes, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) {
        smallrag_pusherrstd();
        return -1;
    }
    embds->vecs = mapping;
    return vec_size;
}

ssize_t smallrag_write_embds(int fd, const struct smallrag_embds *embds)
{
    if (ftruncate(fd, 0) == -1) {
        smallrag_pusherrstd();
        return -1;
    }
    if (lseek(fd, 0, SEEK_SET) == -1) {
        smallrag_pusherrstd();
        return -1;
    }
    size_t vec_size;
    if (__builtin_mul_overflow(embds->dim, embds->cnt, &vec_size)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }
    ssize_t vec_bytes;
    if (__builtin_mul_overflow(vec_size, sizeof(float), &vec_bytes)) {
        smallrag_pusherr(smallragERR_NUM_OVERFLOW);
        return -1;
    }

    const void *src = embds->vecs;
    size_t tot_written = 0;
    while (tot_written < vec_bytes) {
        const ssize_t written = write(fd, src + tot_written, vec_bytes - tot_written);
        if (written == -1) {
            smallrag_pusherrstd();
            return -1;
        }
        tot_written += written;
    }
    return 0;
}   

int *smallrag_get_embds(
        const struct smallrag_embd_provider *provider, 
        size_t full_text_size,
        size_t fragm_cnt, 
        const size_t *fragm_sizes, 
        const char **fragments, 
        struct smallrag_embds *embds)
{
    provider->ops->get_embds(provider, full_text_size, fragm_cnt, fragm_sizes, fragments, embds);
}

void smallrag_embd_provider_free(struct smallrag_embd_provider *provider)
{
    provider->ops->free(provider);
}

