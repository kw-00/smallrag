# Stdio-based information retrieval server.

## Usage:

```text
smallrag -h
smallrag \
   -m <model-path> \
   -d <source-text> \
   -e <embeddings-path> \
   [ -t <n-tokenizer-threads> ] \
   [ -c <context-size> ] \
   [ -b <batch-size> ] \
   [ -u <ubatch-size> ] \
   [ -s <max-seq> ]
```

## Options:

```text
-h - print this help and exit
- model-path - path to .gguf embedding model
- source-text - path to source text. Must exist as a file. Source text should contain text fragments
   delimited using NUL bytes,
- embeddings-path - path to a file that contains/will contain text embeddings. If the file doesn't exist, it embeddings are created for the source text and saved under the path passed via -e.
 if the file does exist, then it is used for retrieval of text fragments.
 Do not attempt to create it yourself - embeddings are managed by the application. Pass a whatever path you want the application to use for storing embeddings.
```

The remaining arguments are used to configure the llama.cpp backend. They are all optional.

```text
- n-tokenizer-threads - number of threads to use for tokenization. Default will use as many threads as there are logical processors.
- context-size, batch-size, ubatch-size - if they are not self-explanatory, I recommend reading about them in the context of AI inference backends or similar. context size/batch size/ubatch size are common terminology that is worth studying separately if you're interested.
- <max-seq> - max number of sequences per logical batch.
```

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
