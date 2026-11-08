#!/bin/bash

export CFLAGS="$CFLAGS $GGUF_MODEL_PATHS_CFLAGS $ONNX_MODEL_PATH_CFLAGS"

sources="embeddings.c embedder.c"
if [ "$BUILD_LLAMA" = true ]; then
    sources+=" llama_embedder.c llama_embedder_params.c"
fi
build_executable_test.sh $sources
