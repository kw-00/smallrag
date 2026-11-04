#!/bin/bash

export CFLAGS="$CFLAGS $GGUF_MODEL_PATHS_CFLAGS"

build_executable.sh llama_embedder.c llama_embedder_params.c
