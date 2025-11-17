# Isabela Server

Isabela Server is a modernized C implementation of the original TestIRC project.
It provides a TCP server that queries FIWARE for student telemetry.
It exposes a client menu to inspect metrics and streams real-time average updates over subscription sockets.
This repository now includes a reproducible build, tests, documentation, and a clean directory layout.

## Repository layout

```
include/        Public headers shared by client, server, and tests
src/server/     Server entrypoint and subsystems (context, data store, network, subscriptions)
src/client/     Modernized interactive CLI client
tests/          Regression tests (data-store smoke test)
fixtures/       Offline sample data for testing without network access
build/          Generated binaries (ignored by Git)
```

## Prerequisites

* GCC/Clang with C11 support
* `make`
* `curl` CLI available in PATH (optional, for live FIWARE downloads)
* POSIX environment (Linux/macOS/WSL).
  Windows builds should use MSYS2 or WSL.

## Building

```
make           # builds build/server and build/client
make test      # compiles and runs tests/test_registry using the fixture data
make clean     # removes build artifacts
```

The build uses `pkg-config` flags when available.
If your toolchain installs `json-c` or `libcurl` in custom locations, export `CFLAGS`/`LDFLAGS` before invoking `make`.

```
CFLAGS="-I/opt/json-c/include" LDFLAGS="-L/opt/json-c/lib" make
```

## Running the server

```
ISABELA_DATA_FILE=fixtures/sample_students.json ./build/server
```

Setting `ISABELA_DATA_FILE` points the server to the offline JSON fixture.
When unset, the server fetches data from the FIWARE endpoint configured in `src/server/data_store.c` by shelling out to the `curl` CLI.
On startup the server prints the listening port (default `9000`).

## Running the client

```
./build/client 127.0.0.1 9000
```

The client guides you through entering your numeric client ID.
It lets you navigate the menu, choose average or single-metric subscriptions, and select a local port for streaming updates.
When a subscription starts, the client forks a helper that connects back to the server on the provided port and prints updates prefixed with `[SUBSCRIPTION]`.
The helper acknowledges each update so the server can push new values.

## Testing and debugging

* `make test` validates the registry loader and average calculator using the fixture data.
  The fixture lives in `fixtures/sample_students.json`.
* Run the server with `ISABELA_DATA_FILE=fixtures/sample_students.json` to avoid relying on remote FIWARE availability during manual tests.
* Use `valgrind` or `asan` by wrapping the binaries, e.g., `valgrind ./build/server`.
* Verbose logging is emitted to stdout/stderr; redirect to files when needed.

## Documentation

Headers and source files now rely on simple inline comments for quick explanations.
Read the headers directly or use your preferred editor tooling to surface the nearby comments.

## Housekeeping

* Compiled binaries live in `build/` and are ignored by Git.
* `make clean` removes artifacts, including object files and tests.
* Report issues or improvement ideas via pull requests.
