#include "smallrag/allocation.h"

#include "smallrag/logging.h"

#include <stdarg.h>
#include <stdlib.h>



int allocate_spans(size_t n_spans, void *pointer_1, size_t sizeof_1, size_t count_1, ...)
{
    static const size_t max_span_count = 1 << 4;
    static const size_t max_va_arg_span_count = max_span_count - 1;

    if (n_spans > max_span_count) {
        LOG_ERROR("Span count too high; up to %zu are allowed.", max_span_count);
        return -1;
    }
    size_t n_va_arg_spans = n_spans -1;

    
    void **pointers[max_va_arg_span_count];
    size_t offsets[max_va_arg_span_count];
    size_t sizes[max_va_arg_span_count];
    size_t counts[max_va_arg_span_count];
    size_t alignments[max_va_arg_span_count];
    {
        va_list ap;
        va_start(ap, count_1);
        for (size_t i = 0; i < n_va_arg_spans; i++) {
            pointers[i] = va_arg(ap, void **);
            sizes[i] = va_arg(ap, size_t);
            counts[i] = va_arg(ap, size_t);
            alignments[i] = va_arg(ap, size_t);
        }
    }
    size_t alloc_size = 0;
    // Set alloc size to match first span
    {
        size_t span_size;
        if (__builtin_mul_overflow(sizeof_1, count_1, &span_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
        if (__builtin_add_overflow(alloc_size, span_size, &alloc_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
    }

    // Increase allocation size separately while itering through the remaining spans.
    // They are treated separately from the first, as they may require alignment 
    for (size_t i = 0; i < n_va_arg_spans; i++) {
        size_t alignment_correction = (alignments[i] - alloc_size % alignments[i]) % alignments[i];
        
        size_t span_size;
        if (__builtin_mul_overflow(sizes[i], counts[i], &span_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }

        if (__builtin_add_overflow(alloc_size, alignment_correction, &alloc_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
        // alloc_size corresponds to current span's aligned location in this moment
        offsets[i] = alloc_size;

        if (__builtin_add_overflow(alloc_size, span_size, &alloc_size)) {
            LOG_ERROR("Number overflow");
            return -1;
        }
    }

    void *memory = malloc(alloc_size);
    if (memory == NULL) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    *(void **)pointer_1 = memory;
    for (size_t i = 0; i < n_va_arg_spans; i++) {
        *pointers[i] = memory + offsets[i];
    }
    return 0;
}


