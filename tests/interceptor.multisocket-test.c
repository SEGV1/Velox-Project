#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Velox interceptor integration test -- multiple simultaneous sockets.
 * Opens two independent TCP connections and exercises both through
 * the interceptor concurrently.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int
open_tcp(const char *ip, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(fd); return -1;
    }
    return fd;
}

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <dest-ip> <dest-port>\n", argv[0]);
        return 1;
    }

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    int fd1 = open_tcp(ip, port);
    int fd2 = open_tcp(ip, port);

    if (fd1 < 0 || fd2 < 0) return 1;
    printf("[multi-sock] opened fd1=%d fd2=%d\n", fd1, fd2);

    const char *msg = "GET / HTTP/1.0\r\n\r\n";

    send(fd1, msg, strlen(msg), 0);
    send(fd2, msg, strlen(msg), 0);

    char buf1[256] = { 0 }, buf2[256] = { 0 };
    ssize_t r1 = recv(fd1, buf1, sizeof(buf1)-1, 0);
    ssize_t r2 = recv(fd2, buf2, sizeof(buf2)-1, 0);

    printf("[multi-sock] fd1 recv %zd bytes\n", r1);
    printf("[multi-sock] fd2 recv %zd bytes\n", r2);

    close(fd1);
    close(fd2);
    return 0;
}
