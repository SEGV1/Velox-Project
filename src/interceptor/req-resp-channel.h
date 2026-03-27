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

#ifndef INTERCEPTOR_AGENTCHANNEL_H
#define INTERCEPTOR_AGENTCHANNEL_H

#include "rpc/c_out/request.pb-c.h"
#include "rpc/c_out/response.pb-c.h"
#include "rpc/rpc.h"

typedef int chan_t;

chan_t open_chan();
void chan_send(chan_t ch, const Request* req);
// When unpacking, protobuf-c creates the Response for you,
// so we can only use parameter Response** to return the Response created by protobuf-c,
// and cannot use Response* to modify the caller's Response
void chan_recv(chan_t ch, Response** resp);
void close_chan(chan_t);

#endif
