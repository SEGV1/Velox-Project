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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "fdpool.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fdchanmap.h"
#include "interceptor.h"
#include "req-resp-channel.h"

extern int* g_chan_table;

int g_startfd;
int g_fd_count;
static int last_fd_assigned;

static void reserve_fd_range();

void
ask_veloxd_fd_range()
{
    chan_t ch = get_proc_chan();
    {
        printf("-- %s:%d: [INFO] asking Veloxd file descriptors range...\n",
               __FILENAME__, __LINE__);
    }

    /*  1. Assemble request */

    Request req = REQUEST__INIT;
    Request__Atstart atstart = REQUEST__ATSTART__INIT;

    req.pid = getpid();

    atstart.progname = program_invocation_short_name;

    req.atstartaction = &atstart;
    req.calling_case = REQUEST__CALLING_ATSTART_ACTION;

    /*  2. Send request */

    chan_send(ch, &req);

    /*  3. Receive response */

    Response* resp = NULL;
    chan_recv(ch, &resp);

    /*  4. Process response */

    g_startfd = resp->atstartaction->startfd;
    g_fd_count = resp->atstartaction->count;
    last_fd_assigned = g_startfd - 1;

    {
        // DEBUG
        assert(g_startfd != 0);
        assert(g_fd_count != 0);
        printf("-- %s:%d: [INFO] veloxd assigned fd range: start=%d count=%d\n",
               __FILENAME__, __LINE__, g_startfd, g_fd_count);
    }

    response__free_unpacked(resp, NULL);

    reserve_fd_range();
}

bool
is_assigned_by_velox(int fd)
{
    return (fd >= g_startfd && fd < g_startfd + g_fd_count);
}

int
next_avail_fd()
{
    assert(last_fd_assigned < g_startfd + g_fd_count - 1);
    return ++last_fd_assigned;
}

static void
reserve_fd_range()
{
    g_chan_table = (int*)malloc(sizeof(int) * g_fd_count);
    memset(g_chan_table, 0, sizeof(int) * g_fd_count);
}
