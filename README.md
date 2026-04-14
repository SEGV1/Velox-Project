# Velox

Velox is a userspace TCP/IP network stack for Linux. It intercepts socket
calls from unmodified binaries — with no source changes required — and routes
them through a daemon (`veloxd`) that implements the full network stack on a
TUN virtual interface.

## Architecture

Velox has three layers:

**1. Interceptor (`velox-posix.so`)**
An `LD_PRELOAD` shared library that overrides the POSIX socket API
(`socket`, `connect`, `close`, `send`/`sendto`, `recv`/`recvfrom`,
`getpeername`, `getsockname`, `setsockopt`, `fcntl`).
Non-`AF_INET` calls fall through to glibc via `dlsym(RTLD_NEXT, ...)`.
`AF_INET`/TCP and `AF_INET`/UDP calls receive a Velox-managed fd from
a range negotiated with `veloxd` at process startup via the `AtStart` RPC.
Each fd maps to a per-socket IPC channel (`chan_t`) stored in `g_chan_table`
indexed by fd offset.

**2. RPC layer (`src/rpc/`)**
All interceptor-to-daemon communication uses Protobuf `proto2` with a
`oneof`-dispatched `Request`/`Response` schema. Every message includes
`pid` so `veloxd` can correctly attribute shared fds across `fork()`ed
processes that inherit the same channel.

**3. Daemon (`veloxd`)**
Receives RPC calls via an event-reactor loop, dispatches to a per-connection
Session state machine (TCP or UDP), and handles actual packet I/O on a
Linux TUN interface. Supports concurrent connections.

## Current Status

| Feature | Status |
|---|---|
| `socket` / `connect` / `close` | ✅ Implemented |
| `send` / `sendto` / `recv` / `recvfrom` | ✅ Implemented |
| `getpeername` / `getsockname` | ✅ Implemented |
| UDP sessions | ✅ Implemented |
| TCP sessions (basic) | ✅ Implemented |
| TCP connection teardown (FIN/RST) | 🚧 In progress |
| `poll` / `select` / `epoll` | 🚧 In progress |
| `fcntl` / `setsockopt` | 🚧 In progress |
| `fork()` channel sharing | 🚧 Designed, partial |

## Build

Dependencies: `cmake`, `protobuf` (C and C++ runtimes), a C++17-capable compiler.

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
# Start the daemon
./veloxd

# Run any dynamically-linked program through the interceptor
LD_PRELOAD=./velox-posix.so curl http://example.com
```

## Tests

Integration tests live under `tests/`. Each covers a specific scenario:

- `interceptor.tcp-test.c` — basic TCP connect/send/recv
- `interceptor.fork-test.c` — inherited fd across `fork()`
- `interceptor.multisocket-test.c` — concurrent sockets in one process
- `interceptor.pthread-test.c` — socket use across threads
- `interceptor.conn-udp-test.c` — connected UDP
- `interceptor.unconn-udp-test.c` — unconnected UDP with `sendto`/`recvfrom`

## Design Notes

The fd range negotiation (`AtStart` RPC) is what makes transparent
interposition safe: `veloxd` assigns a contiguous block of fd integers
(e.g. starting at 4096) that are pre-reserved via `dup2` so the kernel
will never hand them out for unrelated I/O. The interceptor uses this
range to distinguish Velox-managed fds from all other fds (pipes,
files, existing sockets) without any global state that would break
across `fork()` boundaries.
