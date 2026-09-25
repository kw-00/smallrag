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
    size_t len = strlen(content);
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
    struct smallrag_text text;
    text.len = len;
    if (smallrag_mmap_text(fd, &text) == -1) {
        smallrag_throw();
    }

    assert(memcmp(text.text, content, len) == 0 
        && "mmapped smallrag_text.text expected to match contents of file it was mapped to.");

    if (munmap(text.text, len) == -1) {
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
    char *content = "AAAABBBBCCCCDDDD";
    struct smallrag_text text = {
        .text = content,
        .len = strlen(content)
    };

    struct smallrag_text_frags frags;
    smallrag_split_text(&text, 4, 2, &frags);
    assert(frags.count == 7);
#define privASSERT_FRAG_VAL(index, value) \
    do { \
        assert(memcmp(frags.frags[index].text, value, frags.frags[index].len) == 0); \
    } while (false)
    privASSERT_FRAG_VAL(0, "AAAA");
    privASSERT_FRAG_VAL(1, "AABB");
    privASSERT_FRAG_VAL(2, "BBBB");
    privASSERT_FRAG_VAL(3, "BBCC");
    privASSERT_FRAG_VAL(4, "CCCC");
    privASSERT_FRAG_VAL(5, "CCDD");
    privASSERT_FRAG_VAL(6, "DDDD");

    smallrag_split_text(&text, 4, 0, &frags);
    assert(frags.count == 4);
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



