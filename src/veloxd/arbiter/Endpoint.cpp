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

#include "Endpoint.h"

#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>

#include <functional>
#include <utility>

#include "veloxd/Logging.h"
#include "veloxd/arbiter/IoHandle.h"
#include "veloxd/arbiter/agent/AgentChannel.h"
#include "third-party/concurrentqueue/blockingconcurrentqueue.h"

class FrameBuf;

using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using moodycamel::BlockingConcurrentQueue;

struct Endpoint::Impl
{
    enum Endpoint::Type type;

    bool nonblocking;
    int new_packet_notify_pipe[2];
    bool waiting = false;
    std::function<void()> onAsyncNewPacket;
    unique_ptr<IoHandle> sChannel;

    std::weak_ptr<AgentChannel> rrChannel;
    std::shared_ptr<Session> pcb;

    typedef BlockingConcurrentQueue<
        std::pair<struct sockaddr_in, shared_ptr<FrameBuf>>>
        RecvQType;
    RecvQType recvQ;
    static const int recvQlimit = 128;

public:
    void markSChannelAsReadable();
    void sChannelReadCb();
};

Endpoint::Endpoint(enum Type t, bool isnonblocking)
{
    {
        VELOXD_LOG(debug) << "New Socket";
    }

