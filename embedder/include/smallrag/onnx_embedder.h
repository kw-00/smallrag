#ifndef SMALLRAG_ONNX_EMBEDDER_H
#define SMALLRAG_ONNX_EMBEDDER_H

#include "smallrag/embedder_core.h"

struct onnx_embedder_params {
    size_t n_tokenizer_threads;
    /* Maximum number of tokens per padded batch, including special tokens. */
    size_t batch_size;
    size_t max_sequences_per_batch;
};

/* Finds model.onnx in model_path or model_path/onnx, or a single .onnx file in
 * either directory. Requires tokenizer.json and tokenizer_config.json.
 * Available even when the ONNX backend is disabled, for model-path validation.
 * Returns 0 on success, -1 on error. The caller must free model_file on success.
 */
int find_onnx_model_file(const char *model_path, char **model_file);

/* Initializes an embedder owning an ONNX Runtime session and HuggingFace tokenizer.
 * model_path is a model directory. Returns 0 on success, -1 on error.
 */
int init_onnx_embedder(
        struct embedder *embedder,
        const char *model_path,
        const struct onnx_embedder_params *params);

#endif
