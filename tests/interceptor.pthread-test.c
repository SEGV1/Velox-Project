#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/*
 * Velox interceptor integration test -- multi-threaded socket use.
 * Two pthreads share a connected socket: one sends, one receives.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int g_fd = -1;

static void *
recv_thread(void *arg)
{
    (void)arg;
    char buf[512] = { 0 };
    ssize_t got = recv(g_fd, buf, sizeof(buf)-1, 0);
    printf("[pthread-test] recv thread got %zd bytes: %.*s\n",
           got, (int)got, buf);
    return NULL;
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

    g_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(peer));
    peer.sin_family = AF_INET;
    peer.sin_port   = htons((uint16_t)dest_port);
    inet_pton(AF_INET, dest_ip, &peer.sin_addr);
