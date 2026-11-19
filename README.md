# OS Doc — Stage 1

Implement the baseline client/server document service in `client.c` and
`server.c`.

## Stage 1 requirements

- Represent a 10x10 document with words up to 64 characters.
- Read and write words using the commands in `input.txt`.
- Support multiple client IDs (0–9).
- Use POSIX IPC; sockets and third-party libraries are not allowed.
- Validate command-line input and malformed requests without crashing.
- Return clear success or failure responses.
- Clean up IPC resources and write the document to `output.txt` on shutdown.

## Build and test

```sh
make clean
make CFLAGS="-Wall -Wextra -Werror -g"
make test
```

Document the IPC protocol, resource ownership, failure behavior, and test
verification in your implementation notes.
