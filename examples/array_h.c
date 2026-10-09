#include "smallrag/array.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>


DEFINE_ARRAY_TYPE(int_array, int);
DEFINE_ARRAY_TYPE(int_ptr_array, int *)
DEFINE_ARRAY_TYPE(size_array, size_t)

int unsafe(int **number_spans, size_t *number_counts, size_t total_count, int ***transformed)
{
    *transformed = malloc(sizeof(int *) * total_count);
    if (*transformed == NULL) {
        return -1;
    }
    for (size_t i = 0; i < total_count; i++) {
        (*transformed)[i] = malloc(sizeof(int) * number_counts[i]);
        if ((*transformed)[i] == NULL) {
            return -1;
        }
        for (size_t j = 0; j < number_counts[i]; j++) {
            (*transformed)[i][j] = number_spans[i][j] % 33;
        }
    }
    return 0;
}

int safe(int **number_spans, size_t *number_counts, size_t total_count, int ***transformed)
{
    struct int_ptr_array number_spans_array = new_int_ptr_array(number_spans, total_count);
    struct size_array number_counts_array = new_size_array(number_counts, total_count);

    *transformed = malloc(sizeof(int *) * total_count);
    if (*transformed == NULL) {
        return -1;
    }
    struct int_ptr_array transformed_array = new_int_ptr_array(*transformed, total_count);
    
    for (size_t i = 0; i < total_count; i++) {
        size_t number_count = size_array_get(number_counts_array, i);
        int *number_memory = malloc(sizeof(int) * number_count);
        if (number_memory == NULL) {
            return -1;
        }
        int_ptr_array_set(transformed_array, i, number_memory);
        struct int_array transformed_number_array = new_int_array(number_memory, number_count);
       
        int *source_numbers = int_ptr_array_get(number_spans_array, i); 
        struct int_array source_number_array = new_int_array(source_numbers, number_count);

        for (size_t j = 0; j < number_count; j++) {
            int original = int_array_get(source_number_array, j);
            int_array_set(transformed_number_array, j, original % 33);
        }
    }
    return 0;
}

#define SEED 33
#define BOUNDED_RAND() (rand() % (1 << 2))
int main(void) 
{
    srand(SEED);
    size_t total_count = 1 << 4; 
    int **number_spans = malloc(sizeof(int *) * total_count);
    if (number_spans == NULL) {
        return -1;
    }
    size_t *number_counts = malloc(sizeof(size_t) * total_count);
    if (number_counts == NULL) {
        return -1;
    }
    for (size_t i = 0; i < total_count; i++) {
        size_t count = BOUNDED_RAND();
        number_counts[i] = count;
        number_spans[i] = malloc(sizeof(int) * count);
        for (size_t j = 0; j < count; j++) {
            if (number_spans[i] == NULL) {
                printf("Oops...\n");
                return -1;
            }
            number_spans[i][j] = BOUNDED_RAND();
        }
    }
    int **transformed_unsafe;
    if (unsafe(number_spans, number_counts, total_count, &transformed_unsafe) == -1) {
        return -1;
    }
    int **transformed_safe;
    if (safe(number_spans, number_counts, total_count, &transformed_safe) == -1) {
        return -1;
    }

    for (size_t i = 0; i < total_count; i++) {
        for (size_t j = 0; j < number_counts[i]; j++) {
            printf("%d", transformed_unsafe[i][j] == transformed_safe[i][j]);
            printf("Unsafe: %d, safe: %d\n", transformed_unsafe[i][j], transformed_safe[i][j]);
        }
    }
}
        

    
