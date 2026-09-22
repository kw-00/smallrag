# smallrag

## Goal
*smallrag* is a very simple and straightforward CLI tool for chunking, embedding and RAG functionality.

## Installation
1. Make sure you have llama.cpp. Especially, the `llama-embed` command must work.
1. Clone the project and move it to wherever you want its source files to live. I suggest `/opt` for system-wide installation and `~/.local/opt` for user installation.
1. Ensure the `$SMALLRAG_HOME` environment variable points to the project root and that `$SMALLRAG_HOME/bin` is added to PATH.

Based on these instructions, here are two scripts for quick installation. Bear in mind that you still need to set up llama.cpp manually if you don't have it yet.

### Install system-wide
```bash
cwd="$(pwd)"
cd /opt \
&& sudo git clone https://github.com/kw-00/smallrag.git \
&& cd smallrag \
&& echo -e '#!/bin/bash\n\nexport SMALLRAG_HOME='"\"$(pwd)\""'\n\nPATH="${PATH}:${SMALLRAG_HOME}/bin"' | sudo tee /etc/profile.d/smallrag_path.sh > /dev/null
cd "$cwd"
```

### Install for current user
cwd="$(pwd)"
cd ~/.config/opt \
&& git clone https://github.com/kw-00/smallrag.git \
&& cd smallrag \
&& echo -e '\nexport SMALLRAG_HOME='"\"$(pwd)\""'\n\nPATH="${PATH}:${SMALLRAG_HOME}/bin"' > ~/.bashrc
cd "$cwd"

## Components
The tool provides 3 commands.

### smallrag-embed -s <data source file> -m <.gguf embedding model>
`-s` stands for "source" and allows specifying the name of a data source file in `$SMALLRAG_HOME/sources`.
`-m` stands for "model" and allows specifying the name of a .gguf text embedding model in `$SMALLRAG_HOME/models`. 

The source specified under `-s` can be any type of file. It is always interpreted as a normal text file.

Suppose you run `smallrag-embed -s source.txt -m model.gguf`. smallrag will take the file under `$SMALLRAG_HOME/sources/source.txt` and split it into chunnks. The it will vectorize each chunk using the embedding model under `$SMALLRAG_HOME/sources/model.gguf. The vector embeddings get saved to the `$SMALLRAG_HOME/embeddings/source.txt/model.gguf folder. If embeddings for the specified source/model already exist, they are overwritten.

### smallrag-clear -s <data source file> -m <.gguf embedding model>
Removes the embedding folder for the source/model pair specified with `-s` and `-m`.

### smallrag-start -s <data source file> -m <.gguf embedding model>
Starts an HTTP server for RAG for the source/model pair specified with `-s` and `-m`.

It accepts POST JSON requests in the following format:
```json
{
    "top-k": <number>,
    "query": "<query-text>"
}
```

Upon receiving a request, it calculates the vector embedding for `query`, then ranks all chunks by similarity to the query and selects `top-k` chunks by similarity. 

Any selected chunks that were adjacent are merged together as one single chunk.

Then the following response is sent:
```json
{
    "chunks": [
        "<most-relevant-chunk-text>",
        "<second-most-relevant-chunk-text>",
        ...,
        "<k-th-most-relevant-chunk-text>"
    ]
}

