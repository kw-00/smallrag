#ifndef SMALLRAG_ARRAY_H
#define SMALLRAG_ARRAY_H

#include <stddef.h>
#include <assert.h>
#define ARRAY_ASSERT(condition) assert(condition)

/* Macros for declaring and defining a special struct that represents an array, together with
 * functions that allow to operate on it.
 *
 * This kind of array has the advantage that the functions used to operate have buonds-checking.
 *
 * If NDEBUG is defined, then bounds checking is disabled. 
 *
 * DO NOT access the array struct type fields directly. Only use the functions. The field values
 * are not a stable API.
 */
#define DECLARE_ARRAY_TYPE(name, element_type) \
    struct name; \
    \
    struct name new_##name(element_type* elements, size_t length); \
    \
    element_type name##_get(struct name array, size_t index); \
    \
    void name##_set(struct name array, size_t index, element_type value); \
    \
    struct name name##_offset(struct name array, size_t offset); \
    \
    struct name name##_slice(struct name array, size_t start, size_t end); \
    \
    struct name name##_offset_length(struct name array, size_t offset, size_t length); \
    \
    element_type *name##_elements(struct name array); \

#ifndef NDEBUG
#define DEFINE_ARRAY_TYPE(name, element_type) \
    struct name { \
        element_type *const elements; \
        const size_t length; \
    }; \
    \
    struct name new_##name(element_type* elements, size_t length) \
    { \
        return (struct name){ \
            .elements = elements, \
            .length = length \
        }; \
    } \
    \
    element_type name##_get(struct name array, size_t index) \
    { \
        ARRAY_ASSERT(index < array.length && "Index out of bounds"); \
        return array.elements[index]; \
    } \
    \
    void name##_set(struct name array, size_t index, element_type value) \
    { \
        ARRAY_ASSERT(index < array.length && "Index out of bounds"); \
        array.elements[index] = value; \
    } \
    \
    struct name name##_offset(struct name array, size_t offset) \
    { \
        ARRAY_ASSERT(offset < array.length && "Offet out of bounds"); \
        return (struct name){ \
            .elements = array.elements + offset, \
            .length = array.length - offset \
        }; \
    } \
    \
    struct name name##_slice(struct name array, size_t start, size_t end) \
    { \
        ARRAY_ASSERT(start < end && "Slice start must be smaller than end"); \
        ARRAY_ASSERT(end <= array.length && "Slice end cannot be greater than source array length"); \
        return (struct name){ \
            .elements = array.elements + start, \
            .length = end - start \
        }; \
    } \
    \
    struct name name##_offset_length(struct name array, size_t offset, size_t length) \
    { \
        ARRAY_ASSERT(offset < array.length && "Offset out of bounds"); \
        ARRAY_ASSERT(offset + length < array.length && "Length reaches out of bounds"); \
        return (struct name){ \
            .elements = array.elements + offset, \
            .length = length \
        }; \
    } \
    \
    element_type *name##_elements(struct name array) \
    { \
        return array.elements; \
    }

#else
#define DEFINE_ARRAY_TYPE(name, element_type) \
    struct name { \
        element_type *const elements; \
    }; \
    \
    struct name new_##name(element_type* elements, size_t length) \
    { \
        return (struct name){ \
            .elements = elements, \
        }; \
    } \
    \
    element_type name##_get(struct name array, size_t index) \
    { \
        return array.elements[index]; \
    } \
    \
    void name##_set(struct name array, size_t index, element_type value) \
    { \
        array.elements[index] = value; \
    } \
    \
    struct name name##_offset(struct name array, size_t offset) \
    { \
        return (struct name){ \
            .elements = array.elements + offset, \
        }; \
    } \
    \
    struct name name##_slice(struct name array, size_t start, size_t end) \
    { \
        return (struct name){ \
            .elements = array.elements + start, \
        }; \
    } \
    \
    struct name name##_offset_length(struct name array, size_t offset, size_t length) \
    { \
        ARRAY_ASSERT(offset < array.length && "Offset out of bounds"); \
        ARRAY_ASSERT(offset + length < array.length && "Length reaches out of bounds"); \
        return (struct name){ \
            .elements = array.elements + offset, \
        }; \
    } \
    \
    element_type *name##_elements(struct name array) \
    { \
        return array.elements; \
    }
#endif

#endif
