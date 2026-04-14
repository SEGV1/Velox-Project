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

#include "veloxd/Arbiter.h"

#include <boost/thread/thread.hpp>
#include <memory>

#include "veloxd/Logging.h"
#include "veloxd/arbiter/EventReactor.h"
#include "veloxd/arbiter/RpcDispatch.h"
#include "veloxd/arbiter/IoHandle.h"
#include "veloxd/arbiter/Endpoint.h"
#include "veloxd/arbiter/agent/AgentHub.h"
#include "veloxd/arbiter/agent/AgentChannel.h"
// #include "rpc/cpp_out/request.pb.h"
// #include "rpc/cpp_out/response.pb.h"

using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using boost::thread;
using std::vector;

class Request;
class Response;

struct Arbiter::Impl
{
    EventReactor eventloop;
    shared_ptr<AgentHub> masterListener;
    shared_ptr<RpcDispatch> reqHandler;
    std::list<shared_ptr<AgentChannel>>
        rrChannels; // rrChanel -> Socket -> Session
};

// Reason that defaulted ctor/dtor definitions here:
//  https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile/32269374
Arbiter::Arbiter()
{
    _pImpl.reset(new Arbiter::Impl());
}

Arbiter::~Arbiter() = default;

void
Arbiter::init()
{
    /*  1. AgentHub : handle new connection from interceptors */

    shared_ptr<AgentHub> l = make_shared<AgentHub>();
    l->onNewConnection([this](int connfd) {
        VELOXD_LOG(info) << "New conncection from interceptor";

        // New interceptor connection
        // -> New AgentChannel (rrChannel)
        auto rrChannel = make_shared<AgentChannel>(connfd);
        this->addRRChannel(rrChannel);

        // clang-format off
        rrChannel->setOnNewRequestCB
        (
            [this]
            (const std::shared_ptr<AgentChannel>& rrChannel
             , const std::shared_ptr<const Request>& req)
            -> shared_ptr<Response>
            {
              shared_ptr<AgentChannel> holdit = rrChannel;
                VELOXD_LOG(info) << "New Request from interceptor";
                return this->_pImpl->reqHandler->handleRequest(rrChannel, req);
            }
        );
        // clang-format on

        auto sChannel = rrChannel->getIoHandle();
        this->_pImpl->eventloop.addChannel(sChannel);
        sChannel->setOwnerEventReactor(&this->_pImpl->eventloop);
    });

    _pImpl->masterListener = l;
    IoHandle* listenerCh = l->getIoHandle().get();
    EventReactor& eventloop = _pImpl->eventloop;

    // Let eventloop and channel know each other
    eventloop.addChannel(listenerCh);
    listenerCh->setOwnerEventReactor(&eventloop);

    /*  2. RpcDispatch : handle request from interceptors */

    auto handler = make_shared<RpcDispatch>();
    _pImpl->reqHandler = handler;
}

void
Arbiter::start()
{
    VELOXD_LOG(debug) << "Starting event loop thread";
    thread eventLoopThread([this]() {
        LoggingThreadInitializer i;
        i.run();

        VELOXD_LOG(debug) << "Event loop thread begins to work";

        this->_pImpl->masterListener->listen();
        this->_pImpl->eventloop.loop();
    });
    VELOXD_LOG(debug) << "Started event loop thread";
}

void
Arbiter::stop()
{
    _pImpl->eventloop.stop();
}

void
Arbiter::addRRChannel(const std::shared_ptr<AgentChannel>& ch)
{
    _pImpl->rrChannels.push_back(ch);
    VELOXD_LOG(debug) << "Adding a RRChannel";
}

void
Arbiter::removeRRChannel(const std::shared_ptr<AgentChannel>& ch)
{
    _pImpl->rrChannels.remove_if(
        [&ch](const std::shared_ptr<AgentChannel>& chToPred) -> bool {
            if (chToPred.get() == ch.get()) {
                VELOXD_LOG(debug) << "Removing a RRChannel";
                return true;
            }
            return false;
        });
}

EventReactor*
Arbiter::eventLoop()
{
    return &this->_pImpl->eventloop;
}
