#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Velox interceptor integration test -- connected-mode UDP.
 * Exercises: socket(DGRAM), connect, send, recv, getsockname, getpeername.
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
print_addr(const char *label, const struct sockaddr_in *sa)
{
    char buf[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
    printf("%s: %s:%d\n", label, buf, (int)ntohs(sa->sin_port));
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

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    printf("[conn-udp] socket fd=%d\n", fd);

    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port   = htons((uint16_t)dest_port);
    if (inet_pton(AF_INET, dest_ip, &peer.sin_addr) != 1) {
        fprintf(stderr, "invalid address: %s\n", dest_ip);
        return 1;
    }
    if (connect(fd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
        perror("connect"); return 1;
    }

    struct sockaddr_in local;
    socklen_t alen = sizeof(local);
    getsockname(fd, (struct sockaddr *)&local, &alen);
    print_addr("[conn-udp] local ", &local);
    alen = sizeof(peer);
    getpeername(fd, (struct sockaddr *)&peer, &alen);
    print_addr("[conn-udp] remote", &peer);

    const char *msg = "hello from velox";
    ssize_t sent = send(fd, msg, strlen(msg), 0);
    printf("[conn-udp] sent %zd bytes\n", sent);

    char rbuf[512] = { 0 };
    ssize_t got = recv(fd, rbuf, sizeof(rbuf) - 1, 0);
    printf("[conn-udp] recv %zd bytes: %s\n", got, rbuf);

    close(fd);
    return 0;
}
