#include <stddef.h>


/* Signature:
 * int status = allocate_spans(
 *          size_t n_spans,
 *          pointer_to_any_pointer_type pointer_1, size_t sizeof_1, size_t count_1,
 *          pointer_to_any_pointer_type pointer_2, size_t sizeof_2, size_t count_2, size_t alignof_2,
 *          ...
 *          pointer_to_any_pointer_type pointer_n, size_t sizeof_n, size_t count_n, size_t alignof_n);
 *
 * if (status == -1) {
 *      // handle error
 * }
 *
 * Allocates memory for count_1 objects with size of sizeof_1, count_2 objects with size of 
 * sizeof_2, ..., count_n objects with sizeof_n, all in a single memory block, with respect 
 * for alignment needs.
 *
 * Note that alignment is not specified for the first span. This is because the first span starts
 * at the start of the allocated block, which is naturally aligned for any object.
 *
 * Additionally, every pointer_n is placed with correct memory alignment.
 *
 * To free all the memory, call free(pointer_1).
 *
 */
int allocate_spans(size_t n_spans, void *pointer_1, size_t sizeof_1, size_t count_1, ...);
