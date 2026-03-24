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

#ifndef INTERCEPTOR_FDCHANMAP_H
#define INTERCEPTOR_FDCHANMAP_H

#include <stdbool.h>

#include "rpc/c_out/request.pb-c.h"
#include "rpc/c_out/response.pb-c.h"

typedef int chan_t;

void chanmap_init();

// for interceptor use
chan_t fd_chanmap(int fd);
void set_fd_chanmap(int fd, chan_t ch);
void unset_fd_chanmap(int fd, chan_t ch);
chan_t get_proc_chan();

#endif // INTERCEPTOR_FDCHANMAP_H
