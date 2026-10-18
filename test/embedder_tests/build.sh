export CFLAGS="$CFLAGS -DMODEL_PATH=\"\\\"$ROOT_DIR/models/v5-nano-retrieval-F16.gguf\\\"\""

build_executable.sh llama_embedder.c
