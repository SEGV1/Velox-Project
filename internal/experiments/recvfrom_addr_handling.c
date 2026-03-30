/*
 * Address copy helper (fixed version)
 */

#include <stddef.h>
#include <string.h>

int copy_addr_if_present(void *dst, const void *src, size_t n, int has_addr)
{
    if (!has_addr || dst == NULL || src == NULL) {
        return 0;
    }

    memcpy(dst, src, n);
    return 1;
}
