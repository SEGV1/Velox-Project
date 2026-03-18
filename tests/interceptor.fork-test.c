#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Velox interceptor integration test -- fork() channel inheritance.
 * Opens a TCP connection in the parent, then forks. Both parent and
 * child should be able to use the inherited fd through the interceptor.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
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

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port   = htons((uint16_t)dest_port);
    inet_pton(AF_INET, dest_ip, &peer.sin_addr);

    if (connect(fd, (struct sockaddr *)&peer, sizeof(peer)) < 0) {
        perror("connect"); return 1;
    }
    printf("[fork-test] parent connected, fd=%d\n", fd);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* child */
        const char *msg = "hello from child\n";
        ssize_t s = send(fd, msg, strlen(msg), 0);
        printf("[fork-test] child sent %zd bytes\n", s);
        char rbuf[256] = { 0 };
        ssize_t g = recv(fd, rbuf, sizeof(rbuf)-1, 0);
        printf("[fork-test] child recv %zd bytes\n", g);
        close(fd);
        return 0;
    } else {
        /* parent waits */
        int status;
        waitpid(pid, &status, 0);
        printf("[fork-test] child exited with status %d\n",
               WEXITSTATUS(status));
        close(fd);
    }
    return 0;
}
