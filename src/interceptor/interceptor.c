/*
 * Velox, an userspace TCP/IP network stack.
 * Copyright (C) 2026 SEGV1
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "interceptor.h"

#include <dlfcn.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "fdpool.h"
#include "fdchanmap.h"
#include "req-resp-channel.h"
#include "rpc/c_out/request.pb-c.h"
#include "rpc/c_out/response.pb-c.h"

#undef INTERCEPTOR_RETURN__RET_AND_ERRNO
#define INTERCEPTOR_RETURN__RET_AND_ERRNO(callname)                                    \
    do {                                                                               \
        int ret = resp->callname->ret;                                                 \
        if (ret != -1) {                                                               \
            response__free_unpacked(resp, NULL);                                       \
            return ret;                                                                \
        }                                                                              \
                                                                                       \
        assert(resp->callname->has_errno_);                                            \
        /* response__free_unpacked may reset errno on error,             */ \
        /*  so we must not assign errno_ to errno yet                    */\
        int errno_ = resp->callname->errno_;                                           \
        response__free_unpacked(resp, NULL);                                           \
        errno = errno_;                                                                \
        return -1;                                                                     \
    } while (0)

////////////////////////////////////////////////////////////////////////////////
// Functions provided by glibc
//  for interceptor's own use

struct glibc_socket_funcs glibc_funcs;

////////////////////////////////////////////////////////////////////////////////
// Implementations provided by Velox

/* 1. Socket creation/connection/close */

int
socket(int domain, int type, int protocol)
{
    /*  1. Non-INET/TCP, non-INET/UDP socket creation requests
     *     are passed through to glibc for handling
     */

    if (domain != AF_INET) {
        printf("-- %s:%d: [INFO] pass [non-AF_INET] socket\n", __FILENAME__,
               __LINE__);
        return glibc_funcs.socket(domain, type, protocol);
    }

    if (!(type & SOCK_STREAM) && !(type & SOCK_DGRAM)) {
        printf("-- %s:%d: [INFO] pass [non-AF_INET/TCP, non-AF_INET/UDP] "
               "socket\n",
               __FILENAME__, __LINE__);
        return glibc_funcs.socket(domain, type, protocol);
    }

    /*  2. Request Veloxd */

    {
        printf("-- %s:%d: [INFO] calling interceptor/socket {domain: AF_INET, "
               "type: %s}\n",
               __FILENAME__, __LINE__,
               (type & SOCK_STREAM) ? "SOCK_STREAM" : "SOCK_DGRAM");
    }

    Request req = REQUEST__INIT;
    Request__Socket sockCall = REQUEST__SOCKET__INIT;
    Response* resp;

    int connfd;
    chan_t connCh;

    {
        connfd = next_avail_fd();
        connCh = open_chan();
        set_fd_chanmap(connfd, connCh);

        {
            printf(
                "-- %s:%d: [INFO] assigned fd %d to conncection chan_t %lld\n",
                __FILENAME__, __LINE__, connfd, (long long)connCh);
        }
    }

    {
        req.pid = getpid();

        sockCall.domain = domain;
        sockCall.type = type;
        sockCall.protocol = protocol;

        req.socketcall = &sockCall;
        req.calling_case = REQUEST__CALLING_SOCKET_CALL;
    }

    {
        chan_send(connCh, &req);
        chan_recv(connCh, &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_SOCKET_CALL);

        int ret = resp->socketcall->ret;

        // happy path
        if (ret != -1) {
            response__free_unpacked(resp, NULL);
            return connfd;
        }

        // bad path
        assert(resp->socketcall->has_errno_);
        int errno_ = resp->socketcall->errno_;
        response__free_unpacked(resp, NULL);
        errno = errno_;
        return -1;
    }
}

int
connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.connect(sockfd, addr, addrlen);

    // AF_INET only, struct sockaddr_in only
    assert(addrlen == sizeof(struct sockaddr_in));

    {
        printf("-- %s:%d: [INFO] calling interceptor/connect {fd = %d, chan_t "
               "= %lld}\n",
               __FILENAME__, __LINE__, sockfd, (long long)fd_chanmap(sockfd));
    }

    Request req = REQUEST__INIT;
    Request__Connect connCall = REQUEST__CONNECT__INIT;
    Response* resp;

    {
        req.pid = getpid();

        ProtobufCBinaryData addBytes = {.data = (uint8_t*)addr,
                                        .len = addrlen };
        connCall.addr = addBytes;
        req.connectcall = &connCall;
        req.calling_case = REQUEST__CALLING_CONNECT_CALL;
    }

    {
        chan_send(fd_chanmap(sockfd), &req);
        chan_recv(fd_chanmap(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_CONNECT_CALL);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(connectcall);
    }
}

int
close(int fd)
{
    if (!is_assigned_by_velox(fd))
        return glibc_funcs.close(fd);

    {
        printf("-- %s:%d: [INFO] calling interceptor/close {fd = %d, chan_t = "
               "%lld}\n",
               __FILENAME__, __LINE__, fd, (long long)fd_chanmap(fd));
    }

    Request req = REQUEST__INIT;
    Request__Close closeCall = REQUEST__CLOSE__INIT;
    Response* resp;

    {
        req.pid = getpid();
        req.closecall = &closeCall;
        req.calling_case = REQUEST__CALLING_CLOSE_CALL;
    }

    {
        chan_send(fd_chanmap(fd), &req);
        chan_recv(fd_chanmap(fd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_CLOSE_CALL);

        // close coresponding chan_t
        chan_t ch = fd_chanmap(fd);
        unset_fd_chanmap(fd, fd_chanmap(fd));
        close_chan(ch);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(closecall);
    }
}

/* 2. Socket reading/writing */

ssize_t
read(int fd, void* buf, size_t count)
{
    if (!is_assigned_by_velox(fd))
        return glibc_funcs.read(fd, buf, count);

    // forward to recvfrom
    return recvfrom(fd, buf, count, 0, NULL, NULL);
}

ssize_t
write(int fd, const void* buf, size_t count)
{
    if (!is_assigned_by_velox(fd))
        return glibc_funcs.write(fd, buf, count);

    // forward to sendto
    return sendto(fd, buf, count, 0, NULL, 0);
}

ssize_t
send(int sockfd, const void* buf, size_t len, int flags)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.send(sockfd, buf, len, flags);

    // forward to sendto
    return sendto(sockfd, buf, len, flags, NULL, 0);
}

ssize_t
sendto(int sockfd, const void* buf, size_t len, int flags,
       const struct sockaddr* dest_addr, socklen_t addrlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.sendto(sockfd, buf, len, flags, dest_addr, addrlen);

    if (!buf || !len) {
        return 0;
    }

    {
        printf("-- %s:%d: [INFO] calling interceptor/sendto {fd = %d, chan_t "
               "= %lld}\n",
               __FILENAME__, __LINE__, sockfd, (long long)fd_chanmap(sockfd));
    }

    Request req = REQUEST__INIT;
    Request__Sendto sendtoCall = REQUEST__SENDTO__INIT;
    ProtobufCBinaryData bufData;
    ProtobufCBinaryData addrData;
    Response* resp;

    {
        req.pid = getpid();

        bufData.data = (uint8_t*)buf;
        bufData.len = len;

        sendtoCall.buf = bufData;
        sendtoCall.flags = flags;

        if (dest_addr) {
            addrData.data = (uint8_t*)dest_addr;
            assert(addrlen == sizeof(struct sockaddr_in));
            addrData.len = addrlen;
            sendtoCall.has_addr = true;
            sendtoCall.addr = addrData;
        }

        req.sendtocall = &sendtoCall;
        req.calling_case = REQUEST__CALLING_SENDTO_CALL;
    }

    {
        chan_send(fd_chanmap(sockfd), &req);
        chan_recv(fd_chanmap(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_SENDTO_CALL);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(sendtocall);
    }
}

// sendmsg

ssize_t
recv(int sockfd, void* buf, size_t len, int flags)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.recv(sockfd, buf, len, flags);

    // forward to recvfrom
    return recvfrom(sockfd, buf, len, flags, NULL, NULL);
}

ssize_t
recvfrom(int sockfd, void* buf, size_t len, int flags,
         struct sockaddr* src_addr, socklen_t* restrict addrlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

    if (!buf || !len) {
        return 0;
    }

    {
        printf("-- %s:%d: [INFO] calling interceptor/recvfrom {fd = %d, "
               "chan_t = %lld}\n",
               __FILENAME__, __LINE__, sockfd, (long long)fd_chanmap(sockfd));
    }

    Request req = REQUEST__INIT;
    Request__Recvfrom recvfromCall = REQUEST__RECVFROM__INIT;
    Response* resp;

    {
        req.pid = getpid();

        recvfromCall.len = len;
        recvfromCall.flags = flags;
        if (src_addr) {
            recvfromCall.requireaddr = true;
        }

        req.recvfromcall = &recvfromCall;
        req.calling_case = REQUEST__CALLING_RECVFROM_CALL;
    }

    {
        chan_send(fd_chanmap(sockfd), &req);
        chan_recv(fd_chanmap(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_RECVFROM_CALL);

        // Whether there is data to return
        if (resp->recvfromcall->ret > 0) {
            ProtobufCBinaryData buf_bytes = resp->recvfromcall->buf;
            memcpy(buf, buf_bytes.data, buf_bytes.len);
        }

        // Return peer address only when data was actually received and
        // the daemon included one (UDP always does; TCP never does).
        if (src_addr && resp->recvfromcall->ret > 0
                     && resp->recvfromcall->has_addr) {
            // AF_INET only, sockaddr_in only
            ProtobufCBinaryData addr_bytes = resp->recvfromcall->addr;
            memcpy(src_addr, addr_bytes.data, sizeof(struct sockaddr_in));
            *addrlen = sizeof(struct sockaddr_in);
        }

        INTERCEPTOR_RETURN__RET_AND_ERRNO(recvfromcall);
    }
}

// recvmsg

/* 3. Socket polling */

int
poll(struct pollfd* fds, nfds_t nfds, int timeout)
{
    /*
     * Partition the fd array:
     *   - Non-Velox fds go to the real kernel poll.
     *   - Velox-managed fds are reported as immediately ready for the
     *     requested events; full async EventReactor integration is still
     *     in progress (see README).
     */

    /* Use heap allocation instead of VLAs: VLAs are undefined in C++ and
     * have implementation-defined behaviour when nfds == 0 in C.        */
    struct pollfd* non_velox = (struct pollfd*)malloc(nfds * sizeof(*non_velox));
    nfds_t*        nv_idx    = (nfds_t*)malloc(nfds * sizeof(*nv_idx));
    if (!non_velox || !nv_idx) {
        free(non_velox);
        free(nv_idx);
        errno = ENOMEM;
        return -1;
    }
    nfds_t nv_count = 0;

    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (!is_assigned_by_velox(fds[i].fd)) {
            nv_idx[nv_count] = i;
            non_velox[nv_count++] = fds[i];
        }
    }

    /* No Velox fds present — delegate entirely to the kernel. */
    if (nv_count == nfds) {
        free(non_velox);
        free(nv_idx);
        return glibc_funcs.poll(fds, nfds, timeout);
    }

    int nready = 0;

    /*
     * Poll non-Velox fds without blocking: we cannot sleep while Velox
     * fds are in the set, because their data arrives via a separate IPC
     * channel rather than a kernel fd.
     */
    if (nv_count > 0) {
        int r = glibc_funcs.poll(non_velox, nv_count, 0);
        if (r > 0) {
            for (nfds_t i = 0; i < nv_count; i++) {
                fds[nv_idx[i]].revents = non_velox[i].revents;
                if (non_velox[i].revents)
                    nready++;
            }
        }
    }

    /* Report Velox fds as ready for whichever events were requested. */
    for (nfds_t i = 0; i < nfds; i++) {
        if (is_assigned_by_velox(fds[i].fd)) {
            fds[i].revents = fds[i].events & (POLLIN | POLLOUT | POLLHUP);
            if (fds[i].revents)
                nready++;
        }
    }

    free(non_velox);
    free(nv_idx);
    return nready;
}

int
select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
       struct timeval* timeout)
{
    /*
     * Save the caller's fd_sets, then rebuild them to separate
     * Velox-managed fds from non-Velox fds.  Non-Velox fds go to the
     * real kernel select; Velox fds are reported immediately ready
     * (EventReactor integration still in progress).
     */

    fd_set orig_r, orig_w, orig_e;
    if (readfds)   orig_r = *readfds;   else FD_ZERO(&orig_r);
    if (writefds)  orig_w = *writefds;  else FD_ZERO(&orig_w);
    if (exceptfds) orig_e = *exceptfds; else FD_ZERO(&orig_e);

    fd_set nv_r, nv_w, nv_e;
    FD_ZERO(&nv_r); FD_ZERO(&nv_w); FD_ZERO(&nv_e);

    bool has_velox       = false;
    int  max_nv_fd       = -1;

    for (int fd = 0; fd < nfds; fd++) {
        if (is_assigned_by_velox(fd)) {
            if (FD_ISSET(fd, &orig_r) || FD_ISSET(fd, &orig_w) ||
                FD_ISSET(fd, &orig_e))
                has_velox = true;
        } else {
            bool in_any = false;
            if (FD_ISSET(fd, &orig_r)) { FD_SET(fd, &nv_r); in_any = true; }
            if (FD_ISSET(fd, &orig_w)) { FD_SET(fd, &nv_w); in_any = true; }
            if (FD_ISSET(fd, &orig_e)) { FD_SET(fd, &nv_e); in_any = true; }
            if (in_any && fd > max_nv_fd) max_nv_fd = fd;
        }
    }

    /* No Velox fds present — delegate entirely to the kernel. */
    if (!has_velox)
        return glibc_funcs.select(nfds, readfds, writefds, exceptfds, timeout);

    /* Clear the caller's sets; we'll repopulate them below. */
    if (readfds)   FD_ZERO(readfds);
    if (writefds)  FD_ZERO(writefds);
    if (exceptfds) FD_ZERO(exceptfds);

    int nready = 0;

    /* Poll non-Velox fds non-blocking. */
    if (max_nv_fd >= 0) {
        struct timeval zero = {0, 0};
        int r = glibc_funcs.select(max_nv_fd + 1,
                                   &nv_r, &nv_w, &nv_e, &zero);
        if (r > 0) {
            nready += r;
            for (int fd = 0; fd <= max_nv_fd; fd++) {
                if (readfds   && FD_ISSET(fd, &nv_r)) FD_SET(fd, readfds);
                if (writefds  && FD_ISSET(fd, &nv_w)) FD_SET(fd, writefds);
                if (exceptfds && FD_ISSET(fd, &nv_e)) FD_SET(fd, exceptfds);
            }
        }
    }

    /* Mark Velox fds as immediately ready. */
    for (int fd = 0; fd < nfds; fd++) {
        if (is_assigned_by_velox(fd)) {
            bool ready = false;
            if (readfds   && FD_ISSET(fd, &orig_r))
                { FD_SET(fd, readfds);   ready = true; }
            if (writefds  && FD_ISSET(fd, &orig_w))
                { FD_SET(fd, writefds);  ready = true; }
            if (exceptfds && FD_ISSET(fd, &orig_e))
                { FD_SET(fd, exceptfds); ready = true; }
            if (ready)
                nready++;
        }
    }

    return nready;
}

/* 4. Socket options */

int
getpeername(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.getpeername(sockfd, addr, addrlen);

    Request req = REQUEST__INIT;
    Request__Getpeername getpeernameCall = REQUEST__GETPEERNAME__INIT;
    Response* resp;

    {
        req.pid = getpid();

        req.getpeernamecall = &getpeernameCall;
        req.calling_case = REQUEST__CALLING_GETPEERNAME_CALL;
    }

    {
        chan_send(fd_chanmap(sockfd), &req);
        chan_recv(fd_chanmap(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_GETPEERNAME_CALL);
        ProtobufCBinaryData bufData = resp->getpeernamecall->peername;
        memcpy(addr, bufData.data, bufData.len);
        *addrlen = sizeof(struct sockaddr_in);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(getpeernamecall);
    }
}

int
getsockname(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.getsockname(sockfd, addr, addrlen);

    Request req = REQUEST__INIT;
    Request__Getsockname getsocknameCall = REQUEST__GETSOCKNAME__INIT;
    Response* resp;

    {
        req.pid = getpid();

        req.getsocknamecall = &getsocknameCall;
        req.calling_case = REQUEST__CALLING_GETSOCKNAME_CALL;
    }

    {
        chan_send(fd_chanmap(sockfd), &req);
        chan_recv(fd_chanmap(sockfd), &resp);
    }

    {
        assert(resp->returning_case == RESPONSE__RETURNING_GETSOCKNAME_CALL);
        ProtobufCBinaryData bufData = resp->getsocknamecall->sockanme;
        memcpy(addr, bufData.data, bufData.len);
        *addrlen = sizeof(struct sockaddr_in);

        INTERCEPTOR_RETURN__RET_AND_ERRNO(getsocknamecall);
    }
}

int
setsockopt(int sockfd, int level, int optname, const void* optval,
           socklen_t optlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.setsockopt(sockfd, level, optname, optval, optlen);

    /*
     * The daemon does not act on socket-level options (SO_REUSEADDR,
     * SO_SNDBUF, etc.) — the userspace stack manages these internally.
     * Silently succeed so callers that set options before connect/send
     * work transparently.
     */
    return 0;
}

int
getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
    if (!is_assigned_by_velox(sockfd))
        return glibc_funcs.getsockopt(sockfd, level, optname, optval, optlen);

    /*
     * The only option callers typically query on a Velox socket is
     * SOL_SOCKET / SO_ERROR (to check the result of a non-blocking
     * connect).  The daemon clears errors once they have been delivered
     * via a connect/recv response, so we always return 0 here.
     */
    if (level == SOL_SOCKET && optname == SO_ERROR) {
        if (optval && optlen && *optlen >= (socklen_t)sizeof(int)) {
            *(int*)optval = 0;
            *optlen = sizeof(int);
        }
        return 0;
    }

    errno = ENOPROTOOPT;
    return -1;
}

int
fcntl(int fd, int cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    long arg = va_arg(args, long);
    va_end(args);

    if (!is_assigned_by_velox(fd)) {
        return glibc_funcs.fcntl(fd, cmd, arg);
    }

    /*
     * Velox-managed fd.  We only need to support the subset of fcntl
     * commands that well-behaved callers issue on sockets:
     *
     *   F_GETFD / F_SETFD  — FD_CLOEXEC flag.  Velox fds are not real
     *                         kernel fds, so FD_CLOEXEC has no meaning;
     *                         return 0 / succeed silently.
     *
     *   F_GETFL / F_SETFL  — O_NONBLOCK.  The nonblocking mode was
     *                         communicated to the daemon at socket()
     *                         creation time via SOCK_NONBLOCK.  Changing
     *                         it post-creation is not yet propagated;
     *                         return O_RDWR on F_GETFL and succeed
     *                         silently on F_SETFL.
     */
    switch (cmd) {
        case F_GETFD:
            return 0;
        case F_SETFD:
            return 0;
        case F_GETFL:
            return O_RDWR;
        case F_SETFL:
            return 0;
        default:
            errno = EINVAL;
            return -1;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Intercept the startup function to initialize the interceptor

static void
interceptor_init()
{
    /* 1. create/connect/close */
    glibc_funcs.socket = dlsym(RTLD_NEXT, "socket");
    glibc_funcs.connect = dlsym(RTLD_NEXT, "connect");
    glibc_funcs.close = dlsym(RTLD_NEXT, "close");
    /* 2. read/write */
    glibc_funcs.read = dlsym(RTLD_NEXT, "read");
    glibc_funcs.write = dlsym(RTLD_NEXT, "write");
    glibc_funcs.send = dlsym(RTLD_NEXT, "send");
    glibc_funcs.sendto = dlsym(RTLD_NEXT, "sendto");
    glibc_funcs.recv = dlsym(RTLD_NEXT, "recv");
    glibc_funcs.recvfrom = dlsym(RTLD_NEXT, "recvfrom");
    /* 3. poll */
    glibc_funcs.poll = dlsym(RTLD_NEXT, "poll");
    glibc_funcs.select = dlsym(RTLD_NEXT, "select");
    /* 4. options */
    glibc_funcs.getpeername = dlsym(RTLD_NEXT, "getpeername");
    glibc_funcs.getsockname = dlsym(RTLD_NEXT, "getsockname");
    glibc_funcs.getsockopt = dlsym(RTLD_NEXT, "getsockopt");
    glibc_funcs.setsockopt = dlsym(RTLD_NEXT, "setsockopt");
    glibc_funcs.fcntl = dlsym(RTLD_NEXT, "fcntl");

    chanmap_init();
}

int
__libc_start_main(int*(main)(int, char**, char**), int argc, char** ubp_av,
                  void (*init)(void), void (*fini)(void),
                  void (*rtld_fini)(void), void(*stack_end))
{
    interceptor_init();

    // Function pointer glibc_start_main declaration
    int (*glibc_start_main)(int*(main)(int, char**, char**), int argc,
                            char** ubp_av, void (*init)(void),
                            void (*fini)(void), void (*rtld_fini)(void),
                            void(*stack_end)) = NULL;

    glibc_start_main = dlsym(RTLD_NEXT, "__libc_start_main");

    return glibc_start_main(main, argc, ubp_av, init, fini, rtld_fini,
                            stack_end);
}
