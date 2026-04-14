#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Velox interceptor integration test -- unconnected UDP (sendto/recvfrom).
 * Exercises: socket(DGRAM), sendto, recvfrom, getsockname.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <dest-ip> <dest-port>\n", argv[0]);
        return 1;
    }

    const char *dest_ip   = argv[1];
    int         dest_port = atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    printf("[unconn-udp] socket fd=%d\n", fd);

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port   = htons((uint16_t)dest_port);
    if (inet_pton(AF_INET, dest_ip, &dst.sin_addr) != 1) {
        fprintf(stderr, "invalid address: %s\n", dest_ip);
        return 1;
    }

    const char *msg = "hello from velox (unconnected udp)\n";
    ssize_t sent = sendto(fd, msg, strlen(msg), 0,
                          (struct sockaddr *)&dst, sizeof(dst));
    printf("[unconn-udp] sendto %zd bytes\n", sent);

    struct sockaddr_in local;
    socklen_t alen = sizeof(local);
    getsockname(fd, (struct sockaddr *)&local, &alen);
    char lbuf[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &local.sin_addr, lbuf, sizeof(lbuf));
    printf("[unconn-udp] local: %s:%d\n", lbuf, (int)ntohs(local.sin_port));

    char rbuf[512] = { 0 };
    struct sockaddr_in src;
    socklen_t slen = sizeof(src);
    ssize_t got = recvfrom(fd, rbuf, sizeof(rbuf) - 1, 0,
                           (struct sockaddr *)&src, &slen);
    char sbuf[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &src.sin_addr, sbuf, sizeof(sbuf));
    printf("[unconn-udp] recvfrom %s:%d — %zd bytes: %s\n",
           sbuf, (int)ntohs(src.sin_port), got, rbuf);

    close(fd);
    return 0;
}
