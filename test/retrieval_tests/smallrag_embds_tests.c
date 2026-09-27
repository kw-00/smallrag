#include "smallrag/err.h"
#include "smallrag/retrieval.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>

void test_smallrag_mmap_embds(void)
{
    FILE *tmp = tmpfile(); 
    if (tmp == NULL) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    const int fd = fileno(tmp);
    if (fd == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    float content[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    const size_t vec_dim = 2;
    const size_t vec_cnt = 2;
    const size_t count = vec_dim * vec_cnt;
    const size_t len = count * sizeof(float);
    if (fwrite(content, sizeof(float), count, tmp) != count) {
        smallrag_pusherrmsg(smallragERR_STREAM, "Error occurred during write");
        smallrag_throw();
    }
    if (fflush(tmp) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    struct smallrag_embds embds = {
        .vecs = content,
        .dim = vec_dim,
        .cnt = vec_cnt
    };
    ssize_t numbers_mapped = smallrag_mmap_embds(fd, &embds);
    if (numbers_mapped == -1) {
        smallrag_throw();
    }

    assert(memcmp(embds.vecs, content, len) == 0 
            && "mmapped smallrag_embds.vecs expected to match contents of file it was mapped to.");

    if (munmap(embds.vecs, len) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    if (close(fd) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
}
void test_smallrag_write_embds(void)
{
    FILE *tmp = tmpfile(); 
    if (tmp == NULL) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    const int fd = fileno(tmp);
    if (fd == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    float content[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    const size_t vec_dim = 2;
    const size_t vec_cnt = 2;
    const size_t count = vec_dim * vec_cnt;
    const size_t len = count * sizeof(float);
    struct smallrag_embds embds = {
        .vecs = content,
        .dim = vec_dim,
        .cnt = vec_cnt
    };
    ssize_t bytes_written = smallrag_write_embds(fd, &embds);
    if (bytes_written == -1) {
        smallrag_throw();
    }
    assert(bytes_written == len && "Discrepancy between reported write size and expected size");

    float readback[4];
    if (fseek(tmp, 0, SEEK_SET) != 0) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    if (fread(readback, sizeof(float), count, tmp) != count) {
        smallrag_pusherrmsg(smallragERR_STREAM, "Error occurred during read");
        smallrag_throw();
    }

    assert(memcmp(readback, content, len) == 0 
            && "smallrag_write_embds expected to write the embeddings' binary contents to the file.");

    if (close(fd) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
}


int main(void)
{
    test_smallrag_mmap_embds();
    test_smallrag_write_embds();
}
