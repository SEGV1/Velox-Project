/*
 * FD table index helper (fixed version)
 */

#include <stddef.h>

int fd_table_index(int fd, int startfd, int fd_count)
{
    if (fd < startfd || fd >= startfd + fd_count) {
        return -1;
    }
    return fd - startfd;
}
