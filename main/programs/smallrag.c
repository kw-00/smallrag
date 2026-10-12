#include "smallrag/normalization.h"
#include "smallrag/dot_product.h"
#include "smallrag/embedder.h"
#include "smallrag/llama_embedder.h"
#include "smallrag/postprocessing_embedder.h"
#include "smallrag/logging.h"

#include "llama.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

/* -m — model path
 * -d — source texts path
 * -e — embedding path
 * -t — number of tokenizer threads
 * -c — context size
 * -b — batch size
 * -u — ubatch size
 * -s — max sequences per batch 
 */
static char *optstring = "msetcbus";

static size_t parse_size_t_arg()
{
    uintmax_t value = strtoumax(optarg, NULL, 16);
    if (value > SIZE_MAX) {
        perror("strtoumax");
        LOG_ERROR("Number overflow");
        exit(1);
    }
    return value;
}

static void print_required_option_message(char *option_short, char *option_long)
{
    fprintf(stderr, "Option -%s (%s) is required\n", option_short, option_long);
}

static int embedding_postprocessing(struct embeddings *embeddings)
{
    for (size_t i = 0; i < embeddings->embedding_count; i++) {
        if (l2_normalize(embeddings->embeddings + i, embeddings->dimension_count) == -1) {
            LOG_ERROR("Normalization failed");
            return -1;
        }
    }
}

int main(int argc, char *argv[])
{
    char *model_path = NULL;
    char *source_texts_path = NULL;
    char *embedding_path = NULL;
    size_t n_tokenizer_threads = 0;
    size_t context_size = 65536;
    size_t batch_size = 2048;
    size_t ubatch_size = 512;
    size_t max_sequences_per_batch = 0;

    while (true) {
        int option = getopt(argc, argv, optstring);
        if (option == -1) break;
        switch (option) {
            case 'm':
                model_path = optarg;
                break;
            case 'd':
                source_texts_path = optarg;
                break;
            case 'e':
                embedding_path = optarg;
                break;
            case 't': 
                n_tokenizer_threads = parse_size_t_arg();
                break;
            case 'c':
                context_size = parse_size_t_arg();
                break;
            case 'b':
                batch_size = parse_size_t_arg();
                break;
            case 'u':
                ubatch_size = parse_size_t_arg();
                break;
            case 's':
                max_sequences_per_batch = parse_size_t_arg();
                break;
        }
    }

    if (model_path == NULL) {
        print_required_option_message("m", "model path");
        exit(1);
    }
    if (source_texts_path == NULL) {
        print_required_option_message("d", "source texts path");
        exit(1);
    }
    if (embedding_path == NULL) {
        print_required_option_message("e", "embedding path");
        exit(1);
    }
    
    struct llama_model *model;
    {
        struct llama_model_params params = llama_model_default_params();
        model = llama_model_load_from_file(model_path, params);
    }
    struct embedder embedder;
    {
        struct llama_embedder_params params = {
            .n_tokenizer_threads = n_tokenizer_threads,
            .context_size = context_size,
            .batch_size = batch_size,
            .ubatch_size = ubatch_size,
            .max_sequences_per_batch = max_sequences_per_batch
        };
        struct embedder llama_embedder;
        if (init_llama_embedder(&llama_embedder, model, &params) == -1) {
            LOG_ERROR("Failed to initialize embedder");
            exit(1);
        }
        if (init_postprocessing_embedder(
                    &embedder, &llama_embedder, &embedding_postprocessing) == -1) {
            LOG_ERROR("Failed to wrap llama embedder with postprocessing");
            exit(1);
        }
    }
    
    dispose_embedder(&embedder);
    llama_model_free(model); 
}
