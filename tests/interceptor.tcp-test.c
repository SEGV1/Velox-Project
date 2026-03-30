#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Velox interceptor integration test -- TCP basic connectivity.
 * Exercises: socket, connect, getsockname, getpeername, send, recv.
 * Usage: <binary> <dest-ip> <dest-port>
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void
print_addr(const char *label, const struct sockaddr_in *addr)
{
    char buf[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
    printf("%s: %s:%d\n", label, buf, (int)ntohs(addr->sin_port));
}

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <dest-ip> <dest-port>\n", argv[0]);
        return 1;
    }

    const char *dest_ip   = argv[1];
    int         dest_port = atoi(argv[2]);

    /* open socket */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    printf("[tcp-test] socket fd=%d\n", fd);

    /* connect */
    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port   = htons((uint16_t)dest_port);
    if (inet_pton(AF_INET, dest_ip, &peer.sin_addr) != 1) {
        fprintf(stderr, "invalid address: %s\n", dest_ip);
        return 1;
    }
    printf("[tcp-test] connecting to %s:%d\n", dest_ip, dest_port);
    if (connect(fd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
        perror("connect"); return 1;
    }

    /* print resolved addresses */
    struct sockaddr_in local;
    socklen_t alen = sizeof(local);
    getsockname(fd, (struct sockaddr *)&local, &alen);
    print_addr("[tcp-test] local ", &local);
    alen = sizeof(peer);
    getpeername(fd, (struct sockaddr *)&peer, &alen);
    print_addr("[tcp-test] remote", &peer);

    /* send */
    const char *msg = "GET / HTTP/1.0\r\n\r\n";
    ssize_t sent = send(fd, msg, strlen(msg), 0);
    printf("[tcp-test] sent %zd bytes\n", sent);

    /* recv */
    char rbuf[512] = { 0 };
    ssize_t got = recv(fd, rbuf, sizeof(rbuf) - 1, 0);
    printf("[tcp-test] recv %zd bytes\n", got);
    if (got > 0)
        fwrite(rbuf, 1, (size_t)got, stdout);

    close(fd);
