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
