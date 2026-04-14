#!/usr/bin/env sh
# Trace all socket-related syscalls made by curl.
# Useful for verifying which calls the interceptor is hooking.

SYSCALLS="socket,connect,sendto,recvfrom,read,write,close,poll,select"
SYSCALLS="${SYSCALLS},fcntl,getsockopt,setsockopt,getpeername,getsockname"

strace -e "${SYSCALLS}" curl "$@"
