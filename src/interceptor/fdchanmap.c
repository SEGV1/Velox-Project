/*
 * Velox, an userspace TCP/IP network stack.
 * Copyright (C) 2026 SEGV1
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "fdchanmap.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "fdpool.h"
#include "interceptor.h"
#include "req-resp-channel.h"

int* g_chan_table;
extern int g_startfd;
extern int g_fd_count;

static chan_t g_proc_chan;

void
chanmap_init()
{
    // Process-level chan_t
    chan_t ch = open_chan();
    g_proc_chan = ch;
    {
        printf("-- %s:%d: [INFO] create process chan_t {chan_t = %lld}\n",
               __FILENAME__, __LINE__, (long long)ch);
    }

    // reserve fd range
    ask_veloxd_fd_range();
}

chan_t
fd_chanmap(int fd)
{
    return g_chan_table[fd - g_startfd];
}

void
set_fd_chanmap(int fd, chan_t ch)
{
    g_chan_table[fd - g_startfd] = ch;
}

void
unset_fd_chanmap(int fd, chan_t ch)
{
    g_chan_table[fd - g_startfd] = 0;
}

chan_t
get_proc_chan()
{
    return g_proc_chan;
}
