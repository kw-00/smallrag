
#include "smallrag/array.h"

#include <stdbool.h>
#include <stddef.h>
#include <assert.h>


#define SRC_SIZE (1 << 8)
static int memory[2 * SRC_SIZE];
static int *src;
// Prevent UB when reaching out of src bounds by reserving more memory after src
static int *beyond_src;

static bool assertion_failed = false;


#ifndef NDEBUG 
#define ASSERT_ARRAY_ERROR() assert(assertion_failed)
#define RESET_ARRAY_ERROR() \
    do { \
        assertion_failed = false; \
    } while (false)

#else
#define ASSERT_ARRAY_ERROR()
#define RESET_ARRAY_ERROR()
#endif


// Redefine ARRAY_ASSERT, which is called by array functions for bounds checks, to pick up on
// failed checks instead of exiting the program
#undef ARRAY_ASSERT
#define ARRAY_ASSERT(condition) \
    do { \
        if (!condition) assertion_failed = true; \
    } while (false)
DECLARE_ARRAY_TYPE(int_array, int);
DEFINE_ARRAY_TYPE(int_array, int);

static void init_src(void)
{
    src = memory;
    beyond_src = memory + SRC_SIZE;
    for (size_t i = 0;  i < SRC_SIZE; i++) {
        src[i] = i;
    }
}


static void test_access(void)
{
    init_src();
    struct int_array array = new_int_array(src, SRC_SIZE);
    for (size_t i = 0;  i < SRC_SIZE; i++) {
        assert(int_array_get(array, i) == src[i]);
    }

    RESET_ARRAY_ERROR();
    int_array_get(array, SRC_SIZE + 1);
    ASSERT_ARRAY_ERROR();
}

static void test_mutation(void)
{    
    init_src();
    const int factor = 33;
    struct int_array array = new_int_array(src, SRC_SIZE);
    for (size_t i = 0;  i < SRC_SIZE; i++) {
        int_array_set(array, i, i * factor);
    }
    for (size_t i = 0;  i < SRC_SIZE; i++) {
        assert(int_array_get(array, i) == i * factor);
    }
    RESET_ARRAY_ERROR();
    int_array_set(array, SRC_SIZE + 1, 0);
    ASSERT_ARRAY_ERROR();

}

static void test_offset(void)
{
    init_src();
    const size_t offset = 1 << 2;
    struct int_array original = new_int_array(src, SRC_SIZE);

    
    struct int_array with_offset = int_array_offset(original, offset);
    const size_t with_offset_length = SRC_SIZE - offset;
    for (size_t i = 0; i < with_offset_length; i++) {
        assert(int_array_get(with_offset, i) == int_array_get(original, i + offset));
    }
    RESET_ARRAY_ERROR();
    int_array_get(with_offset, with_offset_length + 1); 
    ASSERT_ARRAY_ERROR();
}

static void test_slice(void)
{
    const size_t start = 1 << 2;
    const size_t end = 1 << 6;

    
    struct int_array original = new_int_array(src, SRC_SIZE);

    struct int_array slice = int_array_slice(original, start, end);
    const size_t slice_length = end - start;
    for (size_t i = 0; i < slice_length; i++) {
        assert(int_array_get(slice, i) == int_array_get(original, i + start));
    }
    RESET_ARRAY_ERROR();
    int_array_get(slice, slice_length + 1);
    ASSERT_ARRAY_ERROR();
}

static void test_offset_length(void)
{
    const size_t offset = 1 << 2;
    const size_t length = 1 << 2;

    
    struct int_array original = new_int_array(src, SRC_SIZE);
    
    struct int_array with_offset = int_array_offset_length(original, offset, length);
    for (size_t i = 0; i < length; i++) {
        assert(int_array_get(with_offset, i) == int_array_get(original, i + offset));
    }
    RESET_ARRAY_ERROR();
    int_array_get(with_offset, length + 1);
    ASSERT_ARRAY_ERROR();
}

int main(void)
{
    test_access();
    test_mutation();
    test_offset();
    test_slice();
    test_offset_length();
}

