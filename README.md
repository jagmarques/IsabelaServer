# Isabela Server

Isabela Server is a C client server pair inspired by the TestIRC coursework project.
The server maintains telemetry for students retrieved from FIWARE or local fixtures.
The client connects over TCP, lets a user inspect current metrics, and receives streaming average updates through subscription sockets.

## Repository layout

```
client.c        Interactive CLI entry point
server.c        Server entry point that wires together the server modules
include/        Public headers shared by client, server, and tests
src/server/     Context, data store, network, and subscription modules
tests/          Regression tests that exercise the registry loader
fixtures/       Offline JSON sample data
build/          Generated binaries (ignored by Git)
```

## Requirements

* GCC or Clang with C11 support
* `make`
* `pkg-config` entries for `json-c` and `libcurl` or matching `CFLAGS`/`LDFLAGS`
* POSIX environment such as Linux, macOS, or WSL

## Build instructions

```
make
make test
make clean
```

Set `CFLAGS` and `LDFLAGS` if dependencies live outside standard search paths.

## Running the programs

Start the server with either a remote FIWARE endpoint or the bundled fixture.

```
ISABELA_DATA_FILE=fixtures/sample_students.json ./build/server
```

Launch the client with the host and port printed by the server.

```
./build/client 127.0.0.1 9000
```

The client prompts for the numeric student ID, displays menu options for average queries, and forks helper processes to listen for subscription updates.
The helper processes write each update line to stdout and acknowledge the receipt back to the server.

## Testing and debugging hints

* `make test` runs the regression test against the JSON fixture.
* Use `valgrind` or `asan` when diagnosing memory issues.
* Export `ISABELA_DATA_FILE` during manual testing to avoid network dependencies.

## Maintenance notes

* Artifacts are written to `build/`.
* `make clean` removes binaries, objects, and test executables.
* Contributions and bug reports are welcome through pull requests.
