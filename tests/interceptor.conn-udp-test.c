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

