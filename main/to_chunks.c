#include "smallrag/logging.h"

#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static int open_output(
        char **input_paths,
        size_t input_count,
        const char *output_path,
        FILE **output)
{
    /* Do not truncate the output until it has been checked against every input. */
    int output_fd = open(output_path, O_WRONLY | O_CREAT, 0666);
    if (output_fd == -1) {
        perror("open");
        LOG_ERROR("Failed to open output file \"%s\"", output_path);
        return -1;
    }
#define ERROR_CLEANUP() close(output_fd)

    struct stat output_statbuf;
    if (fstat(output_fd, &output_statbuf) == -1) {
        perror("fstat");
        LOG_ERROR("Failed to stat output file \"%s\"", output_path);
        ERROR_CLEANUP();
        return -1;
    }
    for (size_t i = 0; i < input_count; i++) {
        struct stat input_statbuf;
        if (stat(input_paths[i], &input_statbuf) == -1) {
            perror("stat");
            LOG_ERROR("Failed to stat input file \"%s\"", input_paths[i]);
            ERROR_CLEANUP();
            return -1;
        }
        if (input_statbuf.st_dev == output_statbuf.st_dev
                && input_statbuf.st_ino == output_statbuf.st_ino) {
            LOG_ERROR(
                    "Output file \"%s\" is also input file \"%s\"",
                    output_path, input_paths[i]);
            ERROR_CLEANUP();
            return -1;
        }
    }
    if (ftruncate(output_fd, 0) == -1) {
        perror("ftruncate");
        LOG_ERROR("Failed to truncate output file \"%s\"", output_path);
        ERROR_CLEANUP();
        return -1;
    }

    FILE *stream = fdopen(output_fd, "w");
    if (stream == NULL) {
        perror("fdopen");
        LOG_ERROR("Failed to convert output file descriptor to stream");
        ERROR_CLEANUP();
        return -1;
    }
    *output = stream;
#undef ERROR_CLEANUP
    return 0;
}

static int copy_file(const char *input_path, FILE *output)
{
    FILE *input = fopen(input_path, "rb");
    if (input == NULL) {
        perror("fopen");
        LOG_ERROR("Failed to open input file \"%s\"", input_path);
        return -1;
    }
#define ERROR_CLEANUP() fclose(input)

    unsigned char buffer[BUFSIZ];
    while (true) {
        size_t byte_count = fread(buffer, 1, sizeof(buffer), input);
        if (ferror(input)) {
            perror("fread");
            LOG_ERROR("Failed to read input file \"%s\"", input_path);
            ERROR_CLEANUP();
            return -1;
        }
        if (fwrite(buffer, 1, byte_count, output) != byte_count) {
            perror("fwrite");
            LOG_ERROR("Write failed");
            ERROR_CLEANUP();
            return -1;
        }
        if (byte_count < sizeof(buffer)) {
            break;
        }
    }
    if (fclose(input) == EOF) {
        perror("fclose");
        LOG_ERROR("Failed to close input file \"%s\"", input_path);
        return -1;
    }
#undef ERROR_CLEANUP
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input_file [input_file ...] output_file\n", argv[0]);
        return 1;
    }

    size_t input_count = argc - 2;
    const char *output_path = argv[argc - 1];
    FILE *output;
    if (open_output(argv + 1, input_count, output_path, &output) == -1) {
        return 1;
    }
#define ERROR_CLEANUP() fclose(output)

    for (size_t i = 0; i < input_count; i++) {
        if (copy_file(argv[i + 1], output) == -1) {
            ERROR_CLEANUP();
            return 1;
        }
        if (i + 1 < input_count && fputc('\0', output) == EOF) {
            perror("fputc");
            LOG_ERROR("Failed to write chunk delimiter");
            ERROR_CLEANUP();
            return 1;
        }
    }
    if (fclose(output) == EOF) {
        perror("fclose");
        LOG_ERROR("Failed to close output file \"%s\"", output_path);
        return 1;
    }
#undef ERROR_CLEANUP
    return 0;
}
