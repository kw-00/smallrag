#ifndef SMALLRAG_HF_TOKENIZER_H
#define SMALLRAG_HF_TOKENIZER_H

#include <stddef.h>
#include <stdint.h>

struct hf_tokenizer;

/* Tokens are contiguous, with token_counts[i] tokens for the i-th text.
 * The arrays belong to data and remain valid until dispose_hf_tokens() is called.
 */
struct hf_tokens {
    const int64_t *token_ids;
    const size_t *token_counts;
    size_t text_count;
    void *data;
};

/* Loads tokenizer.json and tokenizer_config.json from model_path.
 * n_threads == 0 uses the number of available logical processors.
 * Returns 0 on success, -1 on error, writing a message to error if provided.
 */
int init_hf_tokenizer(
        struct hf_tokenizer **tokenizer,
        const char *model_path,
        size_t n_threads,
        char *error,
        size_t error_size);

/* Adds the tokenizer's special tokens, without padding or truncation.
 * Does not modify the input texts or lengths. Returns 0 on success, -1 on error.
 */
int tokenize_hf_texts(
        const struct hf_tokenizer *tokenizer,
        const char *const *texts,
        const size_t *text_lengths,
        size_t text_count,
        struct hf_tokens *tokens,
        char *error,
        size_t error_size);

int64_t get_hf_pad_token_id(const struct hf_tokenizer *tokenizer);
int hf_tokenizer_uses_left_padding(const struct hf_tokenizer *tokenizer);
void dispose_hf_tokens(struct hf_tokens *tokens);
void dispose_hf_tokenizer(struct hf_tokenizer *tokenizer);

#endif
