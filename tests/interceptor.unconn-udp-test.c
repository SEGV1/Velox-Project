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

