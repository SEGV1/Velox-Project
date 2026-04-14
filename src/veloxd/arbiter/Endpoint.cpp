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

    _pImpl.reset(new Endpoint::Impl);

    _pImpl->type = t;
    _pImpl->nonblocking = isnonblocking;

    // IoHandle
    pipe2(_pImpl->new_packet_notify_pipe, O_NONBLOCK);

    _pImpl->sChannel.reset(
        new IoHandle(_pImpl->new_packet_notify_pipe[0]));
    _pImpl->sChannel->enableReading();
    _pImpl->sChannel->setOnReadCB(
        std::bind(&Endpoint::Impl::sChannelReadCb, this->_pImpl.get()));
}

Endpoint::~Endpoint()
{
    VELOXD_LOG(debug) << "Destroy Socket";
}

enum Endpoint::Type
Endpoint::type()
{
    return _pImpl->type;
}

bool
Endpoint::nonblocking()
{
    return _pImpl->nonblocking;
}

IoHandle*
Endpoint::getAsyncNewPacketNotifyChannel()
{
    assert(_pImpl->sChannel);

    return _pImpl->sChannel.get();
}

void
Endpoint::setWaiting(bool v)
{
    _pImpl->waiting = v;

    if (v == true) {
        VELOXD_LOG(info) << "Socket waiting state set to true, Socket will "
                               "delegate EventReactor to wait for new async "
                               "packets arrive";
    } else {
        VELOXD_LOG(info) << "Socket waiting state set to false, EventReactor "
                               "will not wait for new async arrives any more";
    }
}

void
Endpoint::setOnAsyncNewPacketCB(std::function<void()> cb)
{
    assert(cb);
    _pImpl->onAsyncNewPacket = cb;
}

void
Endpoint::putToRecvQ(struct sockaddr_in& peeraddr,
                   const std::shared_ptr<FrameBuf>& skbuf)
{
    auto& q = _pImpl->recvQ;

    if (q.size_approx() > Endpoint::Impl::recvQlimit)
        return;

    q.enqueue(std::make_pair(peeraddr, skbuf));
    _pImpl->markSChannelAsReadable();
}

void
Endpoint::takeFromRecvQ(struct sockaddr_in& addr,
                      std::shared_ptr<FrameBuf>& skbuf)
{
    auto pairToPopulate = std::make_pair(std::ref(addr), std::ref(skbuf));

    _pImpl->recvQ.wait_dequeue(pairToPopulate);
}

bool
Endpoint::try_takeFromRecvQ(struct sockaddr_in& addr,
                          std::shared_ptr<FrameBuf>& skbuf)
{
    auto pairToPopulate = std::make_pair(std::ref(addr), std::ref(skbuf));

    return _pImpl->recvQ.try_dequeue(pairToPopulate);
}

weak_ptr<AgentChannel>
Endpoint::reqRespChannel()
{
    return _pImpl->rrChannel;
}

void
Endpoint::setAgentChannel(const std::shared_ptr<AgentChannel>& ch)
{
    assert(ch);
    _pImpl->rrChannel = ch;
}

shared_ptr<Session>
Endpoint::pcb()
{
    return _pImpl->pcb;
}

void
Endpoint::setSession(const std::shared_ptr<Session>& p)
{
    assert(p);
    _pImpl->pcb = p;
}

void
Endpoint::Impl::markSChannelAsReadable()
{
    write(new_packet_notify_pipe[1], "1", 1);
}

void
Endpoint::Impl::sChannelReadCb()
{
    // read until -1 and EAGAIN
    char readbuf[8];
    while (::read(new_packet_notify_pipe[0], readbuf, 8) != -1)
        ;

    if (!this->waiting) {
        return;
    }

    {
        VELOXD_LOG(info) << "New async packet arrives, Socket will pass it "
                               "to processes waitting for it";
    }

    if (onAsyncNewPacket) {
        onAsyncNewPacket();
    }

    this->waiting = false;
}
