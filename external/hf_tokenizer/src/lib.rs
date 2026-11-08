use rayon::{ThreadPool, ThreadPoolBuilder};
use serde_json::Value;
use std::ffi::{c_char, c_int, c_void, CStr};
use std::fs;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::path::Path;
use std::{ptr, slice, str};
use tokenizers::{PaddingDirection, Tokenizer};

pub struct HfTokenizer {
    tokenizer: Tokenizer,
    pool: ThreadPool,
    pad_token_id: i64,
    left_padding: bool,
}

struct TokenStorage {
    token_ids: Vec<i64>,
    token_counts: Vec<usize>,
}

#[repr(C)]
pub struct HfTokens {
    token_ids: *const i64,
    token_counts: *const usize,
    text_count: usize,
    data: *mut c_void,
}

fn ffi_result(
    error: *mut c_char,
    error_size: usize,
    operation: impl FnOnce() -> Result<(), String>,
) -> c_int {
    let result = catch_unwind(AssertUnwindSafe(operation));
    let message = match result {
        Ok(Ok(())) => return 0,
        Ok(Err(message)) => message,
        Err(_) => "HuggingFace tokenizer panicked".to_owned(),
    };
    if !error.is_null() && error_size != 0 {
        let length = message.len().min(error_size - 1);
        unsafe {
            ptr::copy_nonoverlapping(message.as_ptr(), error.cast::<u8>(), length);
            *error.add(length) = 0;
        }
    }
    -1
}

#[no_mangle]
pub unsafe extern "C" fn init_hf_tokenizer(
    out: *mut *mut HfTokenizer,
    model_path: *const c_char,
    n_threads: usize,
    error: *mut c_char,
    error_size: usize,
) -> c_int {
    ffi_result(error, error_size, || {
        if out.is_null() || model_path.is_null() {
            return Err("Tokenizer and model path must not be NULL".to_owned());
        }
        let model_path = CStr::from_ptr(model_path)
            .to_str()
            .map_err(|error| error.to_string())?;
        let model_path = Path::new(model_path);
        let mut tokenizer = Tokenizer::from_file(model_path.join("tokenizer.json"))
            .map_err(|error| error.to_string())?;
        let config: Value = serde_json::from_slice(
            &fs::read(model_path.join("tokenizer_config.json"))
                .map_err(|error| error.to_string())?,
        )
        .map_err(|error| error.to_string())?;
        let padding = tokenizer.get_padding();
        let pad_token = config.get("pad_token").and_then(|token| {
            token
                .as_str()
                .or_else(|| token.get("content").and_then(Value::as_str))
        });
        let pad_token_id = pad_token
            .and_then(|token| tokenizer.token_to_id(token))
            .or_else(|| padding.map(|padding| padding.pad_id))
            .ok_or("Tokenizer configuration must specify a valid pad_token")?;
        let left_padding = match config.get("padding_side").and_then(Value::as_str) {
            Some("left") => true,
            Some("right") => false,
            Some(_) => return Err("Unsupported tokenizer padding_side".to_owned()),
            None => {
                padding.is_some_and(|padding| matches!(padding.direction, PaddingDirection::Left))
            }
        };
        tokenizer.with_padding(None);
        tokenizer
            .with_truncation(None)
            .map_err(|error| error.to_string())?;
        let pool = ThreadPoolBuilder::new()
            .num_threads(n_threads)
            .build()
            .map_err(|error| error.to_string())?;
        *out = Box::into_raw(Box::new(HfTokenizer {
            tokenizer,
            pool,
            pad_token_id: i64::from(pad_token_id),
            left_padding,
        }));
        Ok(())
    })
}

#[no_mangle]
pub unsafe extern "C" fn tokenize_hf_texts(
    tokenizer: *const HfTokenizer,
    texts: *const *const c_char,
    text_lengths: *const usize,
    text_count: usize,
    out: *mut HfTokens,
    error: *mut c_char,
    error_size: usize,
) -> c_int {
    ffi_result(error, error_size, || {
        if tokenizer.is_null() || texts.is_null() || text_lengths.is_null() || out.is_null() {
            return Err("Tokenizer arguments must not be NULL".to_owned());
        }
        if text_count == 0 {
            return Err("text_count must be positive".to_owned());
        }
        let tokenizer = &*tokenizer;
        let texts = slice::from_raw_parts(texts, text_count);
        let lengths = slice::from_raw_parts(text_lengths, text_count);
        let mut inputs = Vec::new();
        inputs
            .try_reserve_exact(text_count)
            .map_err(|error| error.to_string())?;
        for (&text, &length) in texts.iter().zip(lengths) {
            if text.is_null() || length == 0 || length > isize::MAX as usize {
                return Err("Each text must have a valid pointer and positive length".to_owned());
            }
            let bytes = slice::from_raw_parts(text.cast::<u8>(), length);
            inputs.push(str::from_utf8(bytes).map_err(|error| error.to_string())?);
        }
        let encodings = tokenizer
            .pool
            .install(|| tokenizer.tokenizer.encode_batch(inputs, true))
            .map_err(|error| error.to_string())?;
        let token_count = encodings.iter().try_fold(0usize, |total, encoding| {
            total.checked_add(encoding.get_ids().len())
        });
        let token_count = token_count.ok_or("Token count overflow")?;
        let mut storage = Box::new(TokenStorage {
            token_ids: Vec::new(),
            token_counts: Vec::new(),
        });
        storage
            .token_ids
            .try_reserve_exact(token_count)
            .map_err(|error| error.to_string())?;
        storage
            .token_counts
            .try_reserve_exact(text_count)
            .map_err(|error| error.to_string())?;
        for encoding in encodings {
            storage.token_counts.push(encoding.get_ids().len());
            storage
                .token_ids
                .extend(encoding.get_ids().iter().map(|&id| i64::from(id)));
        }
        *out = HfTokens {
            token_ids: storage.token_ids.as_ptr(),
            token_counts: storage.token_counts.as_ptr(),
            text_count,
            data: Box::into_raw(storage).cast::<c_void>(),
        };
        Ok(())
    })
}

#[no_mangle]
pub unsafe extern "C" fn get_hf_pad_token_id(tokenizer: *const HfTokenizer) -> i64 {
    (*tokenizer).pad_token_id
}

#[no_mangle]
pub unsafe extern "C" fn hf_tokenizer_uses_left_padding(tokenizer: *const HfTokenizer) -> c_int {
    c_int::from((*tokenizer).left_padding)
}

#[no_mangle]
pub unsafe extern "C" fn dispose_hf_tokens(tokens: *mut HfTokens) {
    if !tokens.is_null() && !(*tokens).data.is_null() {
        drop(Box::from_raw((*tokens).data.cast::<TokenStorage>()));
        (*tokens).token_ids = ptr::null();
        (*tokens).token_counts = ptr::null();
        (*tokens).text_count = 0;
        (*tokens).data = ptr::null_mut();
    }
}

#[no_mangle]
pub unsafe extern "C" fn dispose_hf_tokenizer(tokenizer: *mut HfTokenizer) {
    if !tokenizer.is_null() {
        drop(Box::from_raw(tokenizer));
    }
}
