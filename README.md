# Smallrag - an stdio-based information retrieval server for Linux (also for Windows via WSL)
## How to install
Smallrag supports llama.cpp and ONNX Runtime embedding backends. Both are enabled by default;
at least one must be enabled. Clone this repository, enter its root and run `./build.sh`.

Environment variables for `build.sh`:
- BUILD_LLAMA — `true` or `false`, default `true`.
- BUILD_ONNX — `true` or `false`, default `true`.
- LLAMA_PATH — llama.cpp repository root; required only when BUILD_LLAMA is `true`.
- LLAMA_BUILD_PATH — optional llama.cpp build folder, default `"$LLAMA_PATH/build"`.
- ONNX_PATH — ONNX Runtime repository or prebuilt SDK root; required only when BUILD_ONNX is `true`.
- ONNX_BUILD_PATH — optional ONNX Runtime build folder. Defaults to `"$ONNX_PATH/build"` for a
  repository, or ONNX_PATH itself for an SDK with `include/` and `lib/`. The runtime library is
  expected in `lib/`, `Linux/Release/`, or directly in that folder.

When using llama.cpp, build it first following
<https://github.com/ggml-org/llama.cpp/blob/master/docs/build.md>.
When using ONNX, install a prebuilt ONNX Runtime SDK or build ONNX Runtime, and install Rust
(`cargo` and `rustc`). The build compiles a small static HuggingFace tokenizer library using Cargo.
Disabled backends do not require their libraries, tooling, or path variables.

For a llama-only build:
```bash
BUILD_ONNX=false LLAMA_PATH='/opt/llama.cpp' ./build.sh
```
For an ONNX-only build with a prebuilt SDK:
```bash
BUILD_LLAMA=false ONNX_PATH='/opt/onnxruntime-linux-x64-1.31.1' ./build.sh
```
To build both, leave the BUILD_* variables unset and provide both LLAMA_PATH and ONNX_PATH.
This builds the entire project, including **smallrag** and **to_chunks**.

You will find the executables in `<project-root>/build/bin`. Add that folder to your PATH to
install them. Keep enabled backends' shared libraries at their configured locations, which are
recorded in the executable's runtime library search path.

## the smallrag program
### Usage

```text
smallrag -h
smallrag -B
smallrag \
   -m <model-path> \
   -d <source-text> \
   -e <embeddings-path> \
   [ -p <query-prefix> ] \
   [ -P <chunk-prefix> ] \
   [ -t <n-tokenizer-threads> ] \
   [ -b <batch-size> ] \
   [ -s <max-seq> ]
```

### Arguments:
- `-B` - print the built backends (`llama`, `onnx`, or `llama,onnx`) and exit without loading a model.
- `model-path` - a .gguf embedding model file or an ONNX model directory. The backend is selected
  automatically. A model requiring a disabled backend produces an informative error.
- `source-text` - comma-separated paths to source text files. Each file must exist and contain text
  fragments delimited using NUL bytes.
- `embeddings-path` - comma-separated paths to files that contain/will contain text embeddings.
  Each path corresponds to the source text path at the same position in `-d`. Both lists must contain
  the same number of paths. If an embedding file doesn't exist or is empty, embeddings are created
  for its source text and saved there. Otherwise, its embeddings are used for retrieval of text fragments.
  Do not attempt to create the embedding files yourself - embeddings are managed by the application.
- `query-prefix` - text prepended directly to each query before embedding. If left unspecified, no prefix is added.
- `chunk-prefix` - text prepended directly to each text chunk before embedding. If left unspecified, no prefix is added. It is not included in responses.

For example, to combine chunks from two source files and their corresponding embeddings:

```bash
smallrag -m model.gguf -d 'a.txt,b.txt' -e 'a.embds,b.embds'
```

Chunks and embeddings are concatenated in the order given. You are responsible for keeping the
source files and embedding files correctly paired, including when changing the order or contents
of source files. Single-file arguments remain supported. Empty filenames are not allowed, and
filenames containing commas are not supported.
Use separate embedding files for different models or backends; existing caches are not regenerated
automatically when the model changes.

An ONNX model directory must contain `tokenizer.json` and `tokenizer_config.json`, plus
`model.onnx` either directly or under `onnx/`. A single differently named .onnx graph is also
accepted when no `model.onnx` exists. Keep any external weight files beside the graph.
The backend supports float32 sentence embeddings and masked mean pooling of `last_hidden_state`,
with dynamic or singleton batch dimensions. The bundled EmbeddingGemma model is used in text-only
mode; its image, video, and audio encoders are not loaded. Model-specific query/chunk prefixes
remain controlled by `-p` and `-P`.

The remaining arguments configure the embedding backend. They are all optional.

- `n-tokenizer-threads` - number of threads to use for tokenization. Default uses as many threads as
  there are logical processors. ONNX Runtime inference also uses this option.
- `batch-size` - maximum total number of tokens in each embedding batch, including special tokens
  and, for ONNX, padding. Each text must fit within this limit; oversized texts are rejected rather
  than truncated. Default: 2048. Larger values can increase memory use.
- `max-seq` - max number of sequences per logical batch.

After building, run `./test.sh` for compiled tests and `./build/test/smallrag_test.sh` for application
tests. Both use the enabled backends; the ONNX test model defaults to `models/embeddinggemma-2-ONNX`
and can be overridden with ONNX_MODEL_PATH when building.

To use smallrag, make sure to enter input into the smallrag process's standard input. The input format is as follows:

```json
{"query": "How many bear species exist in Antarctica?", "top_k": 10}
```

Generally, the input has the form of JSON lines. One line = one query, according to the format above. Splitting the query JSON into multiple lines is an error.

The output format is as follows:

```json
["No known bear species live in Antarctica.", "The grizzly bear is endemic to North America.", ...]
```

Output results are ranked from most relevant to least relevant. For each query, a single line of output containing a JSON list like this is sent via standard ouput.


## the to_chunks program
### Usage
```text
to_chunks input_file [input_file ...] output_file
```

`to_chunks` takes at least two file paths as its arguments. The last file is the **output file**; the others are input files.

It takes the input files, concatenates their contents with delimiting NUL bytes and saves the result to the output file. The output file should not exist.

The output file can then be used as a source text for the smallrag program, as it expects NUL-delimited text fragments.

## Example workflow
First, let's prepare a folder with our source text files. Suppose we're deploying smallrag for animal Q&A. A simplified collection of files might look like this:
```text
sources/mammals.txt
sources/reptiles.txt
sources/insects.txt
```

Now, we could run:
```bash
to_chunks sources/* sources.txt
```
Where *sources.txt* is the output file for `to_chunks`.

Now, suppose we have a .gguf embedding model under model.gguf. let's run:
```bash
smallrag -m model.gguf -d sources.txt -e embeddings
```
Now smallrag is running and listening on standard input. We could enter:
```json
{"query": "Find me some information about mammals", "top_k": 2}
```
smallrag might output:
```json
["Mammals are often cute", "Insects are often a nuisance to mammals"]
```
Assuming the statements in the output list are the content of the two most relevant files among `sources/*`.
