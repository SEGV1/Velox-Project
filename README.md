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
