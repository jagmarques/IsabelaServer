# Isabela Server

Isabela Server is a C-based TCP service inspired by the original TestIRC project. It fetches student telemetry from FIWARE (or a local fixture) and exposes a client interface for inspecting metrics or subscribing to averaged values in real time.

## Features

- Server that keeps a registry of students and their metrics.
- Optional client CLI for navigating metrics and subscriptions.
- Subscription sockets that stream real-time averages with acknowledgement.
- Offline fixtures for deterministic demos and tests.

## Requirements

- POSIX environment (Linux/macOS/WSL recommended)
- C11-compatible compiler (GCC or Clang)
- `make`
- `curl` CLI for live FIWARE downloads (optional)

## Build and Run

```sh
make            # build server and client into build/
make test       # run regression tests against the fixture data
ISABELA_DATA_FILE=fixtures/sample_students.json ./build/server
./build/client 127.0.0.1 9000
```

Set `ISABELA_DATA_FILE` to point the server at the offline JSON fixture when you do not want network calls.

## Project Layout

```
include/        Public headers shared by client, server, and tests
src/server/     Server entrypoint and subsystems (context, data store, network, subscriptions)
src/client/     Interactive CLI client
fixtures/       Sample student data for offline runs
tests/          Regression tests targeting the registry/averages
```

## Testing and Debugging Tips

- `make test` covers the registry loader and average calculations.
- Wrap the binaries with `valgrind` or `asan` for memory diagnostics.
- Verbose logging goes to stdout/stderr; redirect when needed.

## Contributing

Open issues or submit pull requests for enhancements and fixes. Please keep patches focused and well-documented.
