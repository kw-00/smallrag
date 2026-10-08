typedef long int ptrdiff_t;
typedef long unsigned int size_t;
typedef int wchar_t;
typedef struct {
  long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
  long double __max_align_ld
      __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;

extern void __assert_fail(const char *__assertion, const char *__file,
                          unsigned int __line, const char *__function)
    __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
extern void __assert_perror_fail(int __errnum, const char *__file,
                                 unsigned int __line, const char *__function)
    __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));
extern void __assert(const char *__assertion, const char *__file, int __line)
    __attribute__((__nothrow__, __leaf__)) __attribute__((__noreturn__));

static int memory[2 * (1 << 8)];
static int *src;
static int *beyond_src;
static _Bool assertion_failed = 0;
struct int_array;
struct int_array new_int_array(int *elements, size_t length);
int int_array_get(struct int_array array, size_t index);
void int_array_set(struct int_array array, size_t index, int value);
struct int_array int_array_offset(struct int_array array, size_t offset);
struct int_array int_array_slice(struct int_array array, size_t start,
                                 size_t end);
struct int_array int_array_offset_length(struct int_array array, size_t offset,
                                         size_t length);
int *int_array_elements(struct int_array array);
;
struct int_array {
  int *const elements;
  const size_t length;
};
struct int_array new_int_array(int *elements, size_t length) {
  return (struct int_array){.elements = elements, .length = length};
}
int int_array_get(struct int_array array, size_t index) {
  do {
    if (!index < array.length && "Index out of bounds")
      assertion_failed = 1;
  } while (0);
  return array.elements[index];
}
void int_array_set(struct int_array array, size_t index, int value) {
  do {
    if (!index < array.length && "Index out of bounds")
      assertion_failed = 1;
  } while (0);
  array.elements[index] = value;
}
struct int_array int_array_offset(struct int_array array, size_t offset) {
  do {
    if (!offset < array.length && "Offet out of bounds")
      assertion_failed = 1;
  } while (0);
  return (struct int_array){.elements = array.elements + offset,
                            .length = array.length - offset};
}
struct int_array int_array_slice(struct int_array array, size_t start,
                                 size_t end) {
  do {
    if (!start < end && "Slice start must be smaller than end")
      assertion_failed = 1;
  } while (0);
  do {
    if (!end <= array.length &&
        "Slice end cannot be greater than source array length")
      assertion_failed = 1;
  } while (0);
  return (struct int_array){.elements = array.elements + start,
                            .length = end - start};
}
struct int_array int_array_offset_length(struct int_array array, size_t offset,
                                         size_t length) {
  do {
    if (!offset < array.length && "Offset out of bounds")
      assertion_failed = 1;
  } while (0);
  do {
    if (!offset + length < array.length && "Length reaches out of bounds")
      assertion_failed = 1;
  } while (0);
  return (struct int_array){.elements = array.elements + offset,
                            .length = length};
}
int *int_array_elements(struct int_array array) { return array.elements; };
static void init_src(void) {
  src = memory;
  beyond_src = memory + (1 << 8);
  for (size_t i = 0; i < (1 << 8); i++) {
    src[i] = i;
  }
}
static void test_access(void) {
  init_src();
  struct int_array array = new_int_array(src, (1 << 8));
  for (size_t i = 0; i < (1 << 8); i++) {
    ((void)sizeof((int_array_get(array, i) == src[i]) ? 1 : 0), __extension__({
       if (int_array_get(array, i) == src[i])
         ;
       else
         __assert_fail("int_array_get(array, i) == src[i]",
                       "test/data_structures/array.c", 56,
                       __extension__ __PRETTY_FUNCTION__);
     }));
  }
  do {
    assertion_failed = 0;
  } while (0);
  int_array_get(array, (1 << 8) + 1);
  ((void)sizeof((assertion_failed) ? 1 : 0), __extension__({
     if (assertion_failed)
       ;
     else
       __assert_fail("assertion_failed", "test/data_structures/array.c", 61,
                     __extension__ __PRETTY_FUNCTION__);
   }));
}
static void test_mutation(void) {
  init_src();
  const int factor = 33;
  struct int_array array = new_int_array(src, (1 << 8));
  for (size_t i = 0; i < (1 << 8); i++) {
    int_array_set(array, i, i * factor);
  }
  for (size_t i = 0; i < (1 << 8); i++) {
    ((void)sizeof((int_array_get(array, i) == i * factor) ? 1 : 0),
     __extension__({
       if (int_array_get(array, i) == i * factor)
         ;
       else
         __assert_fail("int_array_get(array, i) == i * factor",
                       "test/data_structures/array.c", 73,
                       __extension__ __PRETTY_FUNCTION__);
     }));
  }
  do {
    assertion_failed = 0;
  } while (0);
  int_array_set(array, (1 << 8) + 1, 0);
  ((void)sizeof((assertion_failed) ? 1 : 0), __extension__({
     if (assertion_failed)
       ;
     else
       __assert_fail("assertion_failed", "test/data_structures/array.c", 77,
                     __extension__ __PRETTY_FUNCTION__);
   }));
}
static void test_offset(void) {
  init_src();
  const size_t offset = 1 << 2;
  struct int_array original = new_int_array(src, (1 << 8));
  struct int_array with_offset = int_array_offset(original, offset);
  const size_t with_offset_length = (1 << 8) - offset;
  for (size_t i = 0; i < with_offset_length; i++) {
    ((void)sizeof(
         (int_array_get(with_offset, i) == int_array_get(original, i + offset))
             ? 1
             : 0),
     __extension__({
       if (int_array_get(with_offset, i) == int_array_get(original, i + offset))
         ;
       else
         __assert_fail("int_array_get(with_offset, i) == "
                       "int_array_get(original, i + offset)",
                       "test/data_structures/array.c", 91,
                       __extension__ __PRETTY_FUNCTION__);
     }));
  }
  do {
    assertion_failed = 0;
  } while (0);
  int_array_get(with_offset, with_offset_length + 1);
  ((void)sizeof((assertion_failed) ? 1 : 0), __extension__({
     if (assertion_failed)
       ;
     else
       __assert_fail("assertion_failed", "test/data_structures/array.c", 95,
                     __extension__ __PRETTY_FUNCTION__);
   }));
}
static void test_slice(void) {
  const size_t start = 1 << 2;
  const size_t end = 1 << 6;
  struct int_array original = new_int_array(src, (1 << 8));
  struct int_array slice = int_array_slice(original, start, end);
  const size_t slice_length = end - start;
  for (size_t i = 0; i < slice_length; i++) {
    ((void)sizeof(
         (int_array_get(slice, i) == int_array_get(original, i + start)) ? 1
                                                                         : 0),
     __extension__({
       if (int_array_get(slice, i) == int_array_get(original, i + start))
         ;
       else
         __assert_fail(
             "int_array_get(slice, i) == int_array_get(original, i + start)",
             "test/data_structures/array.c", 109,
             __extension__ __PRETTY_FUNCTION__);
     }));
  }
  do {
    assertion_failed = 0;
  } while (0);
  int_array_get(slice, slice_length + 1);
  ((void)sizeof((assertion_failed) ? 1 : 0), __extension__({
     if (assertion_failed)
       ;
     else
       __assert_fail("assertion_failed", "test/data_structures/array.c", 113,
                     __extension__ __PRETTY_FUNCTION__);
   }));
}
static void test_offset_length(void) {
  const size_t offset = 1 << 2;
  const size_t length = 1 << 2;
  struct int_array original = new_int_array(src, (1 << 8));
  struct int_array with_offset =
      int_array_offset_length(original, offset, length);
  for (size_t i = 0; i < length; i++) {
    ((void)sizeof(
         (int_array_get(with_offset, i) == int_array_get(original, i + offset))
             ? 1
             : 0),
     __extension__({
       if (int_array_get(with_offset, i) == int_array_get(original, i + offset))
         ;
       else
         __assert_fail("int_array_get(with_offset, i) == "
                       "int_array_get(original, i + offset)",
                       "test/data_structures/array.c", 126,
                       __extension__ __PRETTY_FUNCTION__);
     }));
  }
  do {
    assertion_failed = 0;
  } while (0);
  int_array_get(with_offset, length + 1);
  ((void)sizeof((assertion_failed) ? 1 : 0), __extension__({
     if (assertion_failed)
       ;
     else
       __assert_fail("assertion_failed", "test/data_structures/array.c", 130,
                     __extension__ __PRETTY_FUNCTION__);
   }));
}
void main(void) {
  test_access();
  test_mutation();
  test_offset();
  test_slice();
  test_offset_length();
}
