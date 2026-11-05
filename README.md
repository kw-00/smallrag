# Smallrag - an stdio-based information retrieval server for Linux (also for Windows via WSL)
## How to install
First of all, you need a llama.cpp repository on your system. 

You also need to build llama.cpp. Check <https://github.com/ggml-org/llama.cpp/blob/master/docs/build.md> to see how to build llama.cpp.

Next, build **smallrag**. To build smallrag, clone this repository, enter its root and run *build.sh*. 
Two environment variables can be set for `build.sh`:
- LLAMA_PATH — set it to point to your llama.cpp repository root. This is mandatory.
- LLAMA_BUILD_PATH — optional. Set it to point to your llama.cpp build folder. This variable is optional. If it is unset or empty, the smallrag build system will expect a llama.cpp build at `"$LLAMA_PATH/build"`.


To install **smallrag**, clone the smallrag repository, enter its root and run *build.sh* from its root.Supposing you have a llama.cpp repository at `/opt/llama.cpp` and you have built llama.cpp in `/opt/llama.cpp/build`, you could run:
```bash
LLAMA_PATH='/opt/llama.cpp' ./build.sh
```
From repository root. This will build the entire project, including its two executable programs: **smallrag** and **to_chunks**.

Both executables are self-contained. You will find them in `<project-root>/build/bin`. To install them, you only need to add the `<project-root>/build/bin` folder to your PATH.

## the smallrag program
### Usage

```text
smallrag -h
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
- `model-path` - path to .gguf embedding model
- `source-text` - path to source text. Must exist as a file. Source text should contain text fragments
   delimited using NUL bytes,
- `embeddings-path` - path to a file that contains/will contain text embeddings. If the file doesn't exist, it embeddings are created for the source text and saved under the path passed via -e.
 if the file does exist, then it is used for retrieval of text fragments.
 Do not attempt to create it yourself - embeddings are managed by the application. Pass a whatever path you want the application to use for storing embeddings.
- `query-prefix` - text prepended directly to each query before embedding. If left unspecified, no prefix is added.
- `chunk-prefix` - text prepended directly to each text chunk before embedding. If left unspecified, no prefix is added. It is not included in responses.

The remaining arguments are used to configure the llama.cpp backend. They are all optional.

- `n-tokenizer-threads` - number of threads to use for tokenization. Default will use as many threads as there are logical processors.
- `batch-size` - maximum total number of tokens in each embedding batch, including special tokens. Each text must fit within this limit. Default: 2048. Larger values can increase memory use.
- `max-seq` - max number of sequences per logical batch.

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
