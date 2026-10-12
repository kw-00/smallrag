#include "smallrag/data_loading.h"
#include "smallrag/logging.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stddef.h>
#include <stdbool.h>

#define CLOSE_OR_EXIT(fd) \
    do { \
        if (close((fd)) == -1) { \
            perror("close"); \
            LOG_ERROR("Closing file failed"); \
            exit(1); \
        } \
    } while (false)

int map_text(char *text_path, char **text, size_t *text_length)
{
    int fd; 
    off_t file_size;
    {
        fd = open(text_path, O_RDONLY);
        if (fd == -1) {
            perror("open");
            LOG_ERROR("Failed to open texts file");
            return -1;
        }
        struct stat statbuf;
        if (fstat(fd, &statbuf) == -1) {
            perror("fstat");
            LOG_ERROR("Failed to stat texts file");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        file_size = statbuf.st_size;
    }
    if (file_size > SIZE_MAX) {
        LOG_ERROR("Number overflow");
        CLOSE_OR_EXIT(fd);
        return -1;
    }
    void *mapping = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) {
        perror("mmap");
        LOG_ERROR("Mapping failed");
        CLOSE_OR_EXIT(fd);
        return -1;
    }
    *text = mapping;
    *text_length = file_size;
    CLOSE_OR_EXIT(fd);
}

static void dispose_mapped_embeddings(struct embeddings *embeddings)
{
    size_t total_bytes
        = embeddings->embedding_count * embeddings->dimension_count * sizeof(float);
    if (munmap(embeddings->embeddings, total_bytes)) {
        perror("munmap");
        LOG_ERROR("Munmap of embeddings failed");
        exit(1);
    }
}

int load_embeddings(
        char *embeddings_path,
        struct chunks *chunks,
        struct embedder *embedder,
        struct embeddings *embeddings)
{
    int fd = open(embeddings_path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("open");
        LOG_ERROR("Failed to open texts file");
        return -1;
    }
    off_t file_size;
    {
        struct stat statbuf;
        if (fstat(fd, &statbuf) == -1) {
            perror("fstat");
            LOG_ERROR("Failed to stat texts file");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        file_size = statbuf.st_size;
    }

    bool embeddings_cached = file_size > 0;
    if (embeddings_cached) {
        if (file_size > SIZE_MAX) {
            LOG_ERROR("Number overflow");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        if (file_size % sizeof(float) != 0) {
            LOG_ERROR(
                    "Embedding file already exists, but is invalid. Its size indicates that it"
                    " stores something other than just native floating-point numbers");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        size_t total_vector_size = file_size / sizeof(float);
        size_t embedding_count = chunks->count;
        if (total_vector_size % embedding_count != 0) {
            LOG_ERROR(
                    "Loaded embeddings do not fit chunks. The number of floating-point numbers"
                    " it stores is not divisible by the number of chunks");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        size_t dimension_count = total_vector_size / embedding_count;
        void *mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped == MAP_FAILED) {
            perror("mmap");
            LOG_ERROR("Found embedding file, but failed to map it");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        embeddings->embeddings = mapped;
        embeddings->dimension_count = dimension_count;
        embeddings->embedding_count = embedding_count;
        embeddings->dispose = &dispose_mapped_embeddings;
        CLOSE_OR_EXIT(fd);
    } else {
        get_embeddings(embedder, chunks->texts, chunks->sizes, chunks->count, embeddings);
        FILE *stream = fdopen(fd, "w");
        if (stream == NULL) {
            perror("fdopen");
            LOG_ERROR("Failed to convert file descriptor to stream");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        size_t total_vector_size;
        if (__builtin_mul_overflow(
                    embeddings->embedding_count, 
                    embeddings->dimension_count, 
                    &total_vector_size)) {
            LOG_ERROR("Number overflow");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        size_t total_bytes;
        if (__builtin_mul_overflow(
                    total_vector_size,
                    sizeof(float),
                    &total_bytes)) {
            LOG_ERROR("Number overflow");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        if (fwrite(
                    embeddings->embeddings, 
                    total_vector_size, 
                    sizeof(float), stream) < total_vector_size) {
            perror("fwrite");
            LOG_ERROR("Write failed");
            CLOSE_OR_EXIT(fd);
            return -1;
        }
        if (fclose(stream) == EOF) {
            perror("fclose");
            LOG_ERROR("Closing file failed");
            exit(1);
        }
    }
    return 0;
}


