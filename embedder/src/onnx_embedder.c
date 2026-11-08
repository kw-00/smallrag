#include "smallrag/onnx_embedder.h"
#include "smallrag/logging.h"
#include "smallrag/allocation.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *get_file_path(const char *directory, const char *filename)
{
    size_t length;
    if (__builtin_add_overflow(strlen(directory), strlen(filename), &length)
            || __builtin_add_overflow(length, (size_t)2, &length)) {
        LOG_ERROR("Number overflow");
        return NULL;
    }
    char *path = malloc(length);
    if (path == NULL) {
        LOG_ERROR("Allocation failed");
        return NULL;
    }
    strcpy(path, directory);
    strcat(path, "/");
    strcat(path, filename);
    return path;
}

static bool is_readable_file(const char *path)
{
    struct stat info;
    return stat(path, &info) == 0 && S_ISREG(info.st_mode) && access(path, R_OK) == 0;
}

int find_onnx_model_file(const char *model_path, char **model_file)
{
    struct stat info;
    if (stat(model_path, &info) == -1 || !S_ISDIR(info.st_mode)) {
        LOG_ERROR("ONNX model path must be a directory: %s", model_path);
        return -1;
    }
    const char *required_files[] = {"tokenizer.json", "tokenizer_config.json"};
    for (size_t i = 0; i < sizeof(required_files) / sizeof(*required_files); i++) {
        char *path = get_file_path(model_path, required_files[i]);
        if (path == NULL) {
            return -1;
        }
        bool readable = is_readable_file(path);
        free(path);
        if (!readable) {
            LOG_ERROR("ONNX model directory requires a readable %s", required_files[i]);
            return -1;
        }
    }

    char *onnx_directory = get_file_path(model_path, "onnx");
    if (onnx_directory == NULL) {
        return -1;
    }
    const char *directories[] = {model_path, onnx_directory};
    for (size_t i = 0; i < sizeof(directories) / sizeof(*directories); i++) {
        char *path = get_file_path(directories[i], "model.onnx");
        if (path == NULL) {
            free(onnx_directory);
            return -1;
        }
        if (is_readable_file(path)) {
            free(onnx_directory);
            *model_file = path;
            return 0;
        }
        free(path);
    }

    char *candidate = NULL;
    for (size_t i = 0; i < sizeof(directories) / sizeof(*directories); i++) {
        DIR *directory = opendir(directories[i]);
        if (directory == NULL) {
            continue;
        }
        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            size_t length = strlen(entry->d_name);
            if (length < 5 || strcmp(entry->d_name + length - 5, ".onnx") != 0) {
                continue;
            }
            char *path = get_file_path(directories[i], entry->d_name);
            if (path == NULL) {
                closedir(directory);
                free(candidate);
                free(onnx_directory);
                return -1;
            }
            if (!is_readable_file(path)) {
                free(path);
                continue;
            }
            if (candidate != NULL) {
                LOG_ERROR("Multiple ONNX graphs found; name the text graph model.onnx");
                free(path);
                free(candidate);
                closedir(directory);
                free(onnx_directory);
                return -1;
            }
            candidate = path;
        }
        closedir(directory);
    }
    free(onnx_directory);
    if (candidate == NULL) {
        LOG_ERROR("No readable .onnx graph found in model directory: %s", model_path);
        return -1;
    }
    *model_file = candidate;
    return 0;
}

#if BUILD_ONNX

#include "smallrag/hf_tokenizer.h"
#include "onnxruntime_c_api.h"
#include <limits.h>
#include <sys/param.h>

enum input_kind {
    INPUT_IDS,
    ATTENTION_MASK,
    TOKEN_TYPE_IDS,
    POSITION_IDS,
    EMPTY_FEATURES
};

struct onnx_input {
    enum input_kind kind;
    ONNXTensorElementDataType element_type;
    int64_t feature_dimension;
};

struct embedder_data {
    const OrtApi *api;
    OrtEnv *environment;
    OrtSession *session;
    OrtMemoryInfo *memory_info;
    struct hf_tokenizer *tokenizer;
    char **input_names;
    struct onnx_input *inputs;
    size_t input_count;
    char *output_name;
    size_t dimension_count;
    size_t sequence_length;
    bool singleton_batch;
    bool mean_pooling;
    struct onnx_embedder_params params;
};

static int check_status(const OrtApi *api, OrtStatus *status)
{
    if (status == NULL) {
        return 0;
    }
    LOG_ERROR("ONNX Runtime: %s", api->GetErrorMessage(status));
    api->ReleaseStatus(status);
    return -1;
}

static void dispose_data(struct embedder_data *data)
{
    for (size_t i = 0; i < data->input_count; i++) {
        free(data->input_names[i]);
    }
    free(data->input_names);
    free(data->output_name);
    if (data->session != NULL) {
        data->api->ReleaseSession(data->session);
    }
    if (data->memory_info != NULL) {
        data->api->ReleaseMemoryInfo(data->memory_info);
    }
    if (data->environment != NULL) {
        data->api->ReleaseEnv(data->environment);
    }
    dispose_hf_tokenizer(data->tokenizer);
    free(data);
}

static void dispose_embedder(struct embedder *embedder)
{
    dispose_data((struct embedder_data *)embedder->data);
}

static int get_tensor_info(
        const OrtApi *api,
        OrtTypeInfo *type_info,
        ONNXTensorElementDataType *element_type,
        size_t *rank,
        int64_t *dimensions)
{
    const OrtTensorTypeAndShapeInfo *tensor_info;
    if (check_status(api, api->CastTypeInfoToTensorInfo(type_info, &tensor_info)) == -1
            || tensor_info == NULL) {
        LOG_ERROR("Expected an ONNX tensor");
        return -1;
    }
    if (check_status(api, api->GetTensorElementType(tensor_info, element_type)) == -1
            || check_status(api, api->GetDimensionsCount(tensor_info, rank)) == -1) {
        return -1;
    }
    if (*rank < 2 || *rank > 3) {
        LOG_ERROR("Expected a rank-two or rank-three ONNX tensor");
        return -1;
    }
    return check_status(api, api->GetDimensions(tensor_info, dimensions, *rank));
}

static int load_model_inputs(struct embedder_data *data, OrtAllocator *allocator)
{
    const OrtApi *api = data->api;
    size_t input_count;
    if (check_status(api, api->SessionGetInputCount(data->session, &input_count)) == -1) {
        return -1;
    }
    if (input_count == 0 || allocate_spans(
                2,
                &data->input_names, sizeof(char *), input_count,
                &data->inputs, sizeof(struct onnx_input), input_count,
                _Alignof(struct onnx_input)) == -1) {
        LOG_ERROR("Failed to allocate ONNX input metadata");
        return -1;
    }
    bool has_input_ids = false;
    for (size_t i = 0; i < input_count; i++) {
        char *name;
        if (check_status(api, api->SessionGetInputName(data->session, i, allocator, &name)) == -1) {
            return -1;
        }
        data->input_names[i] = strdup(name);
        allocator->Free(allocator, name);
        if (data->input_names[i] == NULL) {
            LOG_ERROR("Allocation failed");
            return -1;
        }
        data->input_count++;
        name = data->input_names[i];
        struct onnx_input *input = data->inputs + i;
        if (strcmp(name, "input_ids") == 0) {
            input->kind = INPUT_IDS;
            has_input_ids = true;
        } else if (strcmp(name, "attention_mask") == 0) {
            input->kind = ATTENTION_MASK;
        } else if (strcmp(name, "token_type_ids") == 0) {
            input->kind = TOKEN_TYPE_IDS;
        } else if (strcmp(name, "position_ids") == 0) {
            input->kind = POSITION_IDS;
        } else if (strcmp(name, "image_features") == 0 || strcmp(name, "video_features") == 0
                || strcmp(name, "audio_features") == 0) {
            input->kind = EMPTY_FEATURES;
        } else {
            LOG_ERROR("Unsupported ONNX model input: %s", name);
            return -1;
        }
        OrtTypeInfo *type_info;
        if (check_status(api, api->SessionGetInputTypeInfo(data->session, i, &type_info)) == -1) {
            return -1;
        }
        size_t rank;
        int64_t dimensions[3];
        int result = get_tensor_info(api, type_info, &input->element_type, &rank, dimensions);
        api->ReleaseTypeInfo(type_info);
        if (result == -1 || rank != 2) {
            LOG_ERROR("ONNX model inputs must be rank-two tensors");
            return -1;
        }
        if (input->kind == EMPTY_FEATURES) {
            if (input->element_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
                    || dimensions[0] > 0 || dimensions[1] <= 0) {
                LOG_ERROR("Unsupported ONNX modality-feature shape or type");
                return -1;
            }
            input->feature_dimension = dimensions[1];
            continue;
        }
        if (input->element_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64
                && input->element_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32) {
            LOG_ERROR("ONNX text inputs must contain int64 or int32 values");
            return -1;
        }
        if (dimensions[0] > 1) {
            LOG_ERROR("Only dynamic or singleton ONNX batch dimensions are supported");
            return -1;
        }
        data->singleton_batch |= dimensions[0] == 1;
        if (dimensions[1] > 0) {
            if (data->sequence_length != 0 && data->sequence_length != (size_t)dimensions[1]) {
                LOG_ERROR("ONNX text input sequence lengths do not match");
                return -1;
            }
            data->sequence_length = dimensions[1];
        }
    }
    if (!has_input_ids) {
        LOG_ERROR("ONNX model requires an input_ids input");
        return -1;
    }
    return 0;
}

static int load_model_output(struct embedder_data *data, OrtAllocator *allocator)
{
    const OrtApi *api = data->api;
    size_t output_count;
    if (check_status(api, api->SessionGetOutputCount(data->session, &output_count)) == -1) {
        return -1;
    }
    int best_priority = 0;
    for (size_t i = 0; i < output_count; i++) {
        char *name;
        if (check_status(api, api->SessionGetOutputName(data->session, i, allocator, &name)) == -1) {
            return -1;
        }
        int priority;
        if (strcmp(name, "sentence_embedding") == 0 || strcmp(name, "text_embeds") == 0
                || strcmp(name, "embeddings") == 0) {
            priority = 3;
        } else if (strcmp(name, "pooler_output") == 0) {
            priority = 2;
        } else if (strcmp(name, "last_hidden_state") == 0) {
            priority = 1;
        } else {
            allocator->Free(allocator, name);
            continue;
        }
        OrtTypeInfo *type_info;
        if (check_status(api, api->SessionGetOutputTypeInfo(data->session, i, &type_info)) == -1) {
            allocator->Free(allocator, name);
            return -1;
        }
        ONNXTensorElementDataType element_type;
        size_t rank;
        int64_t dimensions[3];
        int result = get_tensor_info(api, type_info, &element_type, &rank, dimensions);
        api->ReleaseTypeInfo(type_info);
        if (result == -1 || element_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
                || dimensions[rank - 1] <= 0) {
            allocator->Free(allocator, name);
            continue;
        }
        if (priority > best_priority) {
            char *output_name = strdup(name);
            if (output_name == NULL) {
                allocator->Free(allocator, name);
                LOG_ERROR("Allocation failed");
                return -1;
            }
            free(data->output_name);
            data->output_name = output_name;
            data->dimension_count = dimensions[rank - 1];
            data->mean_pooling = rank == 3;
            best_priority = priority;
        }
        allocator->Free(allocator, name);
    }
    if (best_priority == 0) {
        LOG_ERROR("ONNX model requires a float32 sentence embedding or last_hidden_state output");
        return -1;
    }
    return 0;
}

static int copy_embeddings(
        struct embedder_data *data,
        OrtValue *output,
        size_t batch_count,
        size_t sequence_length,
        const size_t *token_counts,
        float *embeddings)
{
    const OrtApi *api = data->api;
    OrtTensorTypeAndShapeInfo *info;
    if (check_status(api, api->GetTensorTypeAndShape(output, &info)) == -1) {
        return -1;
    }
    size_t rank;
    int64_t dimensions[3];
    ONNXTensorElementDataType element_type;
    int result = 0;
    if (check_status(api, api->GetDimensionsCount(info, &rank)) == -1
            || rank != (data->mean_pooling ? 3u : 2u)
            || check_status(api, api->GetDimensions(info, dimensions, rank)) == -1
            || check_status(api, api->GetTensorElementType(info, &element_type)) == -1
            || element_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
            || dimensions[0] != (int64_t)batch_count
            || dimensions[rank - 1] != (int64_t)data->dimension_count
            || (data->mean_pooling && dimensions[1] != (int64_t)sequence_length)) {
        LOG_ERROR("Unexpected ONNX embedding output shape or type");
        result = -1;
    }
    api->ReleaseTensorTypeAndShapeInfo(info);
    if (result == -1) {
        return -1;
    }
    float *values;
    if (check_status(api, api->GetTensorMutableData(output, (void **)&values)) == -1) {
        return -1;
    }
    if (!data->mean_pooling) {
        memcpy(embeddings, values, batch_count * data->dimension_count * sizeof(float));
        return 0;
    }
    bool left_padding = hf_tokenizer_uses_left_padding(data->tokenizer);
    for (size_t i = 0; i < batch_count; i++) {
        size_t start = left_padding ? sequence_length - token_counts[i] : 0;
        for (size_t dimension = 0; dimension < data->dimension_count; dimension++) {
            double sum = 0;
            for (size_t token = 0; token < token_counts[i]; token++) {
                sum += values[(i * sequence_length + start + token) * data->dimension_count + dimension];
            }
            embeddings[i * data->dimension_count + dimension] = sum / token_counts[i];
        }
    }
    return 0;
}

static int run_batch(
        struct embedder_data *data,
        const int64_t *token_ids,
        const size_t *token_counts,
        size_t batch_count,
        size_t sequence_length,
        float *embeddings)
{
    const OrtApi *api = data->api;
    OrtValue **inputs;
    void **buffers;
    if (allocate_spans(
                2,
                &inputs, sizeof(OrtValue *), data->input_count,
                &buffers, sizeof(void *), data->input_count, _Alignof(void *)) == -1) {
        LOG_ERROR("Allocation failed");
        return -1;
    }
    size_t inputs_initialized = 0;
    size_t buffers_initialized = 0;
    OrtValue *output = NULL;
    int result = -1;
    for (size_t i = 0; i < data->input_count; i++) {
        struct onnx_input input = data->inputs[i];
        int64_t shape[] = {(int64_t)batch_count, (int64_t)sequence_length};
        size_t byte_count;
        if (input.kind == EMPTY_FEATURES) {
            shape[0] = 0;
            shape[1] = input.feature_dimension;
            byte_count = 0;
        } else {
            size_t value_size = input.element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64
                ? sizeof(int64_t) : sizeof(int32_t);
            if (__builtin_mul_overflow(batch_count, sequence_length, &byte_count)
                    || __builtin_mul_overflow(byte_count, value_size, &byte_count)) {
                LOG_ERROR("Number overflow");
                goto cleanup;
            }
        }
        buffers[i] = malloc(MAX(byte_count, (size_t)1));
        if (buffers[i] == NULL) {
            LOG_ERROR("Allocation failed");
            goto cleanup;
        }
        buffers_initialized++;
        if (input.kind != EMPTY_FEATURES) {
            size_t token_offset = 0;
            bool left_padding = hf_tokenizer_uses_left_padding(data->tokenizer);
            for (size_t row = 0; row < batch_count; row++) {
                size_t start = left_padding ? sequence_length - token_counts[row] : 0;
                for (size_t column = 0; column < sequence_length; column++) {
                    bool real_token = column >= start && column - start < token_counts[row];
                    int64_t value = 0;
                    if (input.kind == INPUT_IDS) {
                        value = real_token ? token_ids[token_offset + column - start]
                            : get_hf_pad_token_id(data->tokenizer);
                    } else if (input.kind == ATTENTION_MASK) {
                        value = real_token;
                    } else if (input.kind == POSITION_IDS && real_token) {
                        value = column - start;
                    }
                    size_t index = row * sequence_length + column;
                    if (input.element_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
                        ((int64_t *)buffers[i])[index] = value;
                    } else {
                        if (value > INT32_MAX) {
                            LOG_ERROR("Token value exceeds ONNX int32 input range");
                            goto cleanup;
                        }
                        ((int32_t *)buffers[i])[index] = value;
                    }
                }
                token_offset += token_counts[row];
            }
        }
        if (check_status(api, api->CreateTensorWithDataAsOrtValue(
                        data->memory_info, buffers[i], byte_count, shape, 2,
                        input.element_type, inputs + i)) == -1) {
            goto cleanup;
        }
        inputs_initialized++;
    }
    if (check_status(api, api->Run(
                    data->session, NULL, (const char *const *)data->input_names,
                    (const OrtValue *const *)inputs, data->input_count,
                    (const char *const *)&data->output_name, 1, &output)) == -1) {
        goto cleanup;
    }
    result = copy_embeddings(data, output, batch_count, sequence_length, token_counts, embeddings);

cleanup:
    if (output != NULL) {
        api->ReleaseValue(output);
    }
    for (size_t i = 0; i < inputs_initialized; i++) {
        api->ReleaseValue(inputs[i]);
    }
    for (size_t i = 0; i < buffers_initialized; i++) {
        free(buffers[i]);
    }
    free(inputs);
    return result;
}

static void dispose_embeddings(struct embeddings *embeddings)
{
    free(embeddings->embeddings);
}

static int get_embeddings(
        struct embedder *embedder,
        char **texts,
        size_t *text_lengths,
        size_t text_count,
        struct embeddings *embeddings)
{
    if (text_count == 0) {
        LOG_ERROR("text_count must be positive");
        return -1;
    }
    for (size_t i = 0; i < text_count; i++) {
        if (text_lengths[i] == 0) {
            LOG_ERROR("Each of text_lengths must be positive");
            return -1;
        }
    }
    struct embedder_data *data = (struct embedder_data *)embedder->data;
    struct hf_tokens tokens;
    char error[256];
    if (tokenize_hf_texts(
                data->tokenizer, (const char *const *)texts, text_lengths,
                text_count, &tokens, error, sizeof(error)) == -1) {
        LOG_ERROR("Tokenization failed: %s", error);
        return -1;
    }
    size_t max_length = data->sequence_length != 0 ? data->sequence_length : data->params.batch_size;
    for (size_t i = 0; i < text_count; i++) {
        if (tokens.token_counts[i] == 0 || tokens.token_counts[i] > max_length) {
            LOG_ERROR("Sequence length of %zu is too great for token limit of %zu",
                    tokens.token_counts[i], max_length);
            dispose_hf_tokens(&tokens);
            return -1;
        }
    }
    size_t value_count;
    float *embedding_memory;
    if (__builtin_mul_overflow(text_count, data->dimension_count, &value_count)
            || allocate_spans(1, &embedding_memory, sizeof(float), value_count) == -1) {
        LOG_ERROR("Failed to allocate embeddings");
        dispose_hf_tokens(&tokens);
        return -1;
    }
    size_t text_index = 0;
    size_t token_offset = 0;
    while (text_index < text_count) {
        size_t batch_count = 0;
        size_t sequence_length = data->sequence_length;
        size_t batch_tokens = 0;
        size_t max_sequences = data->singleton_batch ? 1 : data->params.max_sequences_per_batch;
        while (text_index + batch_count < text_count && batch_count < max_sequences) {
            size_t count = tokens.token_counts[text_index + batch_count];
            size_t padded_length = MAX(sequence_length, count);
            if (batch_count + 1 > data->params.batch_size / padded_length) {
                break;
            }
            batch_count++;
            batch_tokens += count;
            sequence_length = padded_length;
        }
        if (run_batch(data, tokens.token_ids + token_offset, tokens.token_counts + text_index,
                    batch_count, sequence_length,
                    embedding_memory + text_index * data->dimension_count) == -1) {
            dispose_hf_tokens(&tokens);
            free(embedding_memory);
            return -1;
        }
        text_index += batch_count;
        token_offset += batch_tokens;
    }
    dispose_hf_tokens(&tokens);
    embeddings->embeddings = embedding_memory;
    embeddings->dimension_count = data->dimension_count;
    embeddings->embedding_count = text_count;
    embeddings->dispose = &dispose_embeddings;
    return 0;
}

int init_onnx_embedder(
        struct embedder *embedder,
        const char *model_path,
        const struct onnx_embedder_params *params)
{
    if (params->batch_size == 0 || params->batch_size > INT32_MAX
            || params->max_sequences_per_batch == 0 || params->max_sequences_per_batch > UINT32_MAX
            || params->n_tokenizer_threads > INT_MAX) {
        LOG_ERROR("Invalid ONNX embedder batch, sequence or thread count");
        return -1;
    }
    char *model_file;
    if (find_onnx_model_file(model_path, &model_file) == -1) {
        return -1;
    }
    const OrtApi *api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (api == NULL) {
        LOG_ERROR("ONNX Runtime does not support C API version %d", ORT_API_VERSION);
        free(model_file);
        return -1;
    }
    struct embedder_data *data = malloc(sizeof(struct embedder_data));
    if (data == NULL) {
        LOG_ERROR("Allocation failed");
        free(model_file);
        return -1;
    }
    data->api = api;
    data->environment = NULL;
    data->session = NULL;
    data->memory_info = NULL;
    data->tokenizer = NULL;
    data->input_names = NULL;
    data->input_count = 0;
    data->output_name = NULL;
    data->sequence_length = 0;
    data->singleton_batch = false;
    data->params = *params;
    char error[256];
    if (init_hf_tokenizer(&data->tokenizer, model_path, params->n_tokenizer_threads,
                error, sizeof(error)) == -1) {
        LOG_ERROR("Failed to initialize tokenizer: %s", error);
        free(model_file);
        dispose_data(data);
        return -1;
    }
    OrtSessionOptions *options;
    if (check_status(api, api->CreateSessionOptions(&options)) == -1) {
        free(model_file);
        dispose_data(data);
        return -1;
    }
    int result = 0;
    if (check_status(api, api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "smallrag", &data->environment)) == -1
            || check_status(api, api->SetSessionGraphOptimizationLevel(options, ORT_ENABLE_ALL)) == -1
            || check_status(api, api->SetIntraOpNumThreads(options, params->n_tokenizer_threads)) == -1
            || check_status(api, api->CreateSession(data->environment, model_file, options, &data->session)) == -1
            || check_status(api, api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &data->memory_info)) == -1) {
        result = -1;
    }
    api->ReleaseSessionOptions(options);
    free(model_file);
    if (result == -1) {
        dispose_data(data);
        return -1;
    }
    OrtAllocator *allocator;
    if (check_status(api, api->GetAllocatorWithDefaultOptions(&allocator)) == -1
            || load_model_inputs(data, allocator) == -1
            || load_model_output(data, allocator) == -1) {
        dispose_data(data);
        return -1;
    }
    if (data->sequence_length > params->batch_size) {
        LOG_ERROR("ONNX model sequence length exceeds the configured batch size");
        dispose_data(data);
        return -1;
    }
    embedder->data = data;
    embedder->get_embeddings = &get_embeddings;
    embedder->dispose = &dispose_embedder;
    return 0;
}

#endif
