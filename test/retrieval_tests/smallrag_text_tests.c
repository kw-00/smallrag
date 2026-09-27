#include "smallrag/err.h"
#include "smallrag/retrieval.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>

void test_smallrag_mmap_text(void)
{
    FILE *tmp = tmpfile(); 
    if (tmp == NULL) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    char *content = "AAAABBBBCCCCDDDD";
    if (fputs(content, tmp) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    if (fflush(tmp) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    const int fd = fileno(tmp);
    if (fd == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    char *text;
    ssize_t mapped_len = smallrag_mmap_text(fd, &text);
    if (mapped_len == -1) {
        smallrag_throw();
    }

    assert(memcmp(text, content, mapped_len) == 0 
        && "mmapped smallrag_text.text expected to match contents of file it was mapped to.");

    assert(strlen(text) == mapped_len);

    if (munmap(text, mapped_len) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
    if (close(fd) == -1) {
        smallrag_pusherrstd();
        smallrag_throw();
    }
}

void test_smallrag_split_text(void)
{
    char *src = "AAAA\0BBBB\0CCCC\0DDDD";
    size_t src_len = strlen(src);

    size_t *fragm_sizes;
    char **fragms;
    ssize_t cnt = smallrag_split_text(src, src_len, '\0', &fragm_sizes, &fragms);
    if (cnt == -1) {
        smallrag_throw();
    }
    assert(cnt == 4 && "Returned undexpected number of fragments");
#define privASSERT_FRAG_VAL(index, value) \
        assert(memcmp(fragms[index], value, fragm_sizes[index]) == 0);
    privASSERT_FRAG_VAL(0, "AAAA");
    privASSERT_FRAG_VAL(1, "BBBB");
    privASSERT_FRAG_VAL(2, "CCCC");
    privASSERT_FRAG_VAL(3, "DDDD");

    src = "\0\0\0AAAA\0BBBB\0\0\0CCCC\0DDDD";
    src_len = strlen(src);
    cnt = smallrag_split_text(src, src_len, '\0', &fragm_sizes, &fragms);
    if (cnt == -1) {
        smallrag_throw();
    }
    assert(cnt == 4);
    privASSERT_FRAG_VAL(0, "AAAA");
    privASSERT_FRAG_VAL(1, "BBBB");
    privASSERT_FRAG_VAL(2, "CCCC");
    privASSERT_FRAG_VAL(3, "DDDD");
#undef privASSERT_FRAG_VAL
}

int main(void)
{
    test_smallrag_mmap_text();
    test_smallrag_split_text();
}



