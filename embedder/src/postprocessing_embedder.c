#include "smallrag/postprocessing_embedder.h"

#include "smallrag/logging.h"
#include <stdlib.h>

struct postprocessing_embedder_data {
    struct embedder internal_embedder;
    int (*postprocessing_function)(struct embeddings *);
};

static int get_embeddings(
        struct embedder *embedder,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings)
{
    struct postprocessing_embedder_data *embedder_data 
        = (struct postprocessing_embedder_data *)embedder->data;
    struct embedder internal_embedder = embedder_data->internal_embedder;
    if (internal_embedder.get_embeddings(
            &internal_embedder, texts, text_lengths, text_count, embeddings) == -1) {
        return -1;
    }
    if (embedder_data->postprocessing_function(embeddings) == -1) {
        return -1;
    }
    return 0;
}

static void dispose_embedder(struct embedder *embedder)
{

    struct postprocessing_embedder_data *embedder_data 
        = (struct postprocessing_embedder_data *)embedder->data;
    struct embedder internal_embedder = embedder_data->internal_embedder;
    internal_embedder.dispose(&internal_embedder);
    free(embedder_data);
}
            

int init_postprocessing_embedder(
        struct embedder *restrict postprocessing_embedder, 
        struct embedder *restrict wrapped_embedder, 
        int (*postprocessing_function)(struct embeddings *))
{
    struct postprocessing_embedder_data *embedder_data 
        = malloc(sizeof(struct postprocessing_embedder_data));
    if (embedder_data == NULL) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    embedder_data->internal_embedder = *wrapped_embedder;
    embedder_data->postprocessing_function = postprocessing_function;
    postprocessing_embedder->data = embedder_data;

    postprocessing_embedder->get_embeddings = &get_embeddings;
    postprocessing_embedder->dispose = &dispose_embedder;
    return 0;
}
