llama_path="$1"
if [[ -z $llama_path ]]; then
    echo "Usage: $0 path/to/llama.cpp/repo" >&2
    exit 1
fi
LLAMA_INSTALL_PATH="./installations/llama.cpp"

HIPCXX="$(hipconfig -l)/clang" HIP_PATH="$(hipconfig -R)" \
    cmake -S "$llama_path" -B "$llama_path/build" -DGGML_HIP=ON -DGPU_TARGETS=gfx1100 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$LLAMA_INSTALL_PATH" \
    && cmake --build "$llama_path/build" --config Release -- -j 16 && cmake --install "$llama_path/build"

LLAMA_LIB_PATH="$LLAMA_INSTALL_PATH/lib"

patchelf --set-rpath '$ORIGIN' "$LLAMA_LIB_PATH/libllama.so.0"
patchelf --set-rpath '$ORIGIN' "$LLAMA_LIB_PATH/libggml.so.0"

