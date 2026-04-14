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

#include "RpcDispatch.h"

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "veloxd/Logging.h"
#include "veloxd/Arbiter.h"
#include "veloxd/Session.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/Tcp.h"
#include "veloxd/Udp.h"
#include "veloxd/arbiter/EventReactor.h"
#include "veloxd/arbiter/IoHandle.h"
#include "veloxd/arbiter/Endpoint.h"
#include "veloxd/arbiter/agent/AgentChannel.h"
#include "rpc/cpp_out/request.pb.h"
#include "rpc/cpp_out/response.pb.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::weak_ptr;
using std::make_shared;

// Drain a linked FrameBuf chain into a protobuf string field.
// Replaces the repeated inline iteration pattern throughout this file.
static void
drain_skbuf_chain(const shared_ptr<FrameBuf>& head, std::string* out)
{
    for (auto skb = head; skb; skb = skb->next) {
        const int len = skb->user_payload_end - skb->user_payload_begin;
        if (len > 0)
            out->append(skb->user_payload_begin, static_cast<size_t>(len));
    }
}

shared_ptr<Response>
RpcDispatch::handleRequest(const shared_ptr<AgentChannel>& rrChannel,
                              const shared_ptr<const Request>& req)
{
    auto resp = make_shared<Response>();

    switch (req->calling_case()) {
        case Request::kSocketCall:
            VELOXD_LOG(info) << "Handling socketCall...";
            socketCall(rrChannel, req, resp);
            break;
        case Request::kConnectCall:
            VELOXD_LOG(info) << "Handling connectCall...";
            connectCall(rrChannel, req, resp);
            break;
        case Request::kCloseCall:
            VELOXD_LOG(info) << "Handling closeCall...";
            closeCall(rrChannel, req, resp);
            break;
        case Request::kRecvfromCall:
            VELOXD_LOG(info) << "Handling recvfromCall...";
            recvfromCall(rrChannel, req, resp);
            break;
        case Request::kSendtoCall:
            VELOXD_LOG(info) << "Handling sendtoCall...";
            sendtoCall(rrChannel, req, resp);
            break;
        case Request::kPollCall: // in progress: EventReactor integration pending
            break;
        case Request::kSelectCall: // in progress: EventReactor integration pending
            break;
        case Request::kGetpeernameCall:
            VELOXD_LOG(info) << "Handling getpeernameCall...";
            getpeernameCall(rrChannel, req, resp);
            break;
        case Request::kGetsocknameCall:
            VELOXD_LOG(info) << "Handling getsocknameCall...";
            getsocknameCall(rrChannel, req, resp);
            break;
        case Request::kGetsockoptCall:
            VELOXD_LOG(info) << "Handling getsockoptCall...";
            getsockoptCall(rrChannel, req, resp);
            break;
        case Request::kSetsockoptCall:
            VELOXD_LOG(info) << "Handling setsockoptCall...";
            setsockoptCall(rrChannel, req, resp);
            break;
        case Request::kFcntlCall:
            VELOXD_LOG(info) << "Handling fcntlCall...";
            fcntlCall(rrChannel, req, resp);
            break;

        case Request::kAtstartAction:
            VELOXD_LOG(info) << "Handling atstartAction...";
            atstartAction(rrChannel, req, resp);
            break;

        case Request::CALLING_NOT_SET:
            VELOXD_LOG(warning) << "Request is empty";
            break;
        default:
            VELOXD_LOG(warning) << "Unknown Request type";
            break;
    }

    return resp;
}

void
RpcDispatch::socketCall(const shared_ptr<AgentChannel>& rrChannel,
                           const shared_ptr<const Request>& req,
                           const shared_ptr<Response>& resp)
{
    const auto& call = req->socketcall();
    int32_t domain = call.domain();
    int32_t type = call.type();
    // int32_t protocol = call.protocol(); // Ignore it

    assert(domain == AF_INET);
    assert(type & SOCK_STREAM || type & SOCK_DGRAM);


    /*  1. Create Socket and Session */

    shared_ptr<Endpoint> so;
    shared_ptr<Session> pcb;

    bool nonblocking = type & SOCK_NONBLOCK;

    if (type & SOCK_STREAM) {
        so = make_shared<Endpoint>(Endpoint::Type::TCP, nonblocking);
        pcb = Tcp::get()->newSession();
    } else if (type & SOCK_DGRAM) {
        so = make_shared<Endpoint>(Endpoint::Type::UDP, nonblocking);
        pcb = Udp::get()->newSession();
    }

    /*  2. Make RRChannel, Socket, and Session point to each other */

    pcb->setSocket(so);
    so->setSession(pcb);
    so->setAgentChannel(rrChannel);
    rrChannel->setSocket(so);

    /*  3. Add Socket.AsyncNewNotifyChannel to the EventReactor */

    IoHandle* sChannel = so->getAsyncNewPacketNotifyChannel();
    EventReactor* eventloop = Arbiter::get()->eventLoop();
    eventloop->addChannel(sChannel);
    sChannel->setOwnerEventReactor(eventloop);

    /*  4. Add Session.ConnectionEstabedNitifyChannel to the EventReactor */

    sChannel = pcb->getConnEstabNotifyChannel();
    if (sChannel) {
        EventReactor* eventloop = Arbiter::get()->eventLoop();
        eventloop->addChannel(sChannel);
        sChannel->setOwnerEventReactor(eventloop);
    }

    /*  5. Return information to the Interceptor */

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto* callRet = resp->mutable_socketcall();
    callRet->set_ret(0);
}

void
RpcDispatch::connectCall(const shared_ptr<AgentChannel>& rrChannel,
                            const shared_ptr<const Request>& req,
                            const shared_ptr<Response>& resp)
{
    const std::string& destAddr_str = req->connectcall().addr();
    const char* destAddr_bytes = destAddr_str.c_str();
    sockaddr_in destAddr;
    memcpy(&destAddr, destAddr_bytes, sizeof(struct sockaddr_in));

    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();

    /*  0. UDP or TCP ? */

    if (socket->type() == Endpoint::Type::UDP)
        goto udp;
    else
        goto tcp;

udp:
    /*  1. UDP */

    {
        int ret = pcb->connect(destAddr.sin_addr, destAddr.sin_port);
        auto* callRet = resp->mutable_connectcall();
        if (ret == 0) {
            callRet->set_ret(0);
        } else {
            callRet->set_ret(-1);
            callRet->set_errno_(ret);
        }
    }
    return;

tcp:
    /*  2. TCP */

    // - EINPROGRESS: The socket is nonblocking and the connection
    //                cannot be completed immediately. (TCP only)
    if (socket->nonblocking()) {
        goto tcp_in_progress;
    } else {
        goto tcp_wait_estab;
    }

tcp_in_progress:;
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto callRet = resp->mutable_recvfromcall();
        callRet->set_ret(EINPROGRESS);
    }
    return;

tcp_wait_estab:;
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_WAIT_NEXT);

        const auto& socket = rrChannel->socket();
        const auto& pcb = socket->pcb();

        std::weak_ptr<AgentChannel> rrChannel_weak = rrChannel;
        pcb->setOnConnEstabCB([rrChannel_weak]() {
            shared_ptr<AgentChannel> rrChannel = rrChannel_weak.lock();
            if (!rrChannel)
                return;

            rrChannel->socket()
                ->pcb()
                ->getConnEstabNotifyChannel()
                ->unregisterSelf();

            auto resp = make_shared<Response>();
            auto* callRet = resp->mutable_connectcall();

            resp->set_retcode(Response::RetCode::Response_RetCode_OK);
            callRet->set_ret(0);

            rrChannel->write(resp);
        });
        pcb->connect(destAddr.sin_addr, destAddr.sin_port);
    }
    return;
}

void
RpcDispatch::closeCall(const shared_ptr<AgentChannel>& rrChannel,
                          const shared_ptr<const Request>& req,
                          const shared_ptr<Response>& resp)
{

    // /*  1. Decrease reference count; destroy when it reaches 0 */

    // rrChannel->decreaseRefCount();
    // if (!rrChannel->refCount()) {
    //     Arbiter::get()->removeRRChannel(rrChannel);
    // }

    // /*  2. Return information to the Interceptor */

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto* callRet = resp->mutable_closecall();
    callRet->set_ret(0);
}

void
RpcDispatch::sendtoCall(const shared_ptr<AgentChannel>& rrChannel,
                           const shared_ptr<const Request>& req,
                           const shared_ptr<Response>& resp)
{
    const auto& call = req->sendtocall();
    const std::string& buf = call.buf();
    // int32_t flags = call.flags(); // unused
    bool hasAddr = call.has_addr();

    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();
    int nwritten = 0;

    {
        if (socket->type() == Endpoint::Type::UDP) {
            VELOXD_LOG(info) << "It's an UDP send request, passing it to "
                                   "corresponding UdpSession...";
            if (hasAddr) {
                const char* destAddr_bytes = call.addr().c_str();
                struct sockaddr_in destAddr;
                bzero(&destAddr, sizeof(struct sockaddr_in));
                memcpy(&destAddr, destAddr_bytes, sizeof(struct sockaddr_in));

                nwritten = pcb->send(destAddr.sin_addr, destAddr.sin_port, buf);
            } else {
                nwritten = pcb->send(buf);
            }
        } else if (socket->type() == Endpoint::Type::TCP) {
            VELOXD_LOG(info) << "It's a TCP send request, passing it to "
                                   "corresponding TcpSession...";
            nwritten = pcb->send(buf);
        }
    }

    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_sendtocall();
        callRet->set_ret(nwritten);
    }
}

void
RpcDispatch::recvfromCall(const shared_ptr<AgentChannel>& rrChannel,
                             const shared_ptr<const Request>& req,
                             const shared_ptr<Response>& resp)
{
    const auto& call = req->recvfromcall();
    const auto& socket = rrChannel->socket();
    // int32_t flags = call.flags(); // ignored
    bool require_addr = call.requireaddr();

    sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof(sockaddr_in));
    shared_ptr<FrameBuf> skbuf_head;

    {
        if (socket->nonblocking()) {
            if (socket->try_takeFromRecvQ(peeraddr, skbuf_head)) {
                // happy path
                // fall through
            } else {
                // bad path
                goto e_again;
            }
        } else {
            if (socket->try_takeFromRecvQ(peeraddr, skbuf_head)) {
                // happy path
                // fall through
            } else {
                // bad path
                goto wait_next;
            }
        }
    }

    // happy path
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_recvfromcall();
        std::string* buf = callRet->mutable_buf();

        drain_skbuf_chain(skbuf_head, buf);

        if (require_addr) {
            // UDP only
            callRet->set_addr(
                std::string{ (char*)&peeraddr, sizeof(sockaddr_in) });
        }

        callRet->set_ret(buf->length());
        return;
    }

e_again:;
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto callRet = resp->mutable_recvfromcall();
        callRet->set_ret(EAGAIN);
        return;
    }

wait_next:;
    {
        resp->set_retcode(Response::RetCode::Response_RetCode_WAIT_NEXT);

        {
            VELOXD_LOG(info) << "No packet currently available, EventReactor "
                                   "will wait for it. This function will "
                                   "return";
        }

        const auto& socket = rrChannel->socket();
        // Use weak_ptr to prevent Socket and AgentChannel from holding shared_ptrs
        // to each other (which would form a reference cycle).
        //
        // The intended design goal is: Arbiter holds shared_ptr<AgentChannel> and
        //   exclusively manages the AgentChannel lifecycle;
        //   AgentChannel holds shared_ptr<Endpoint> and exclusively manages the
        //   Socket lifecycle.
        //
        // Because std::bind copies its arguments, passing a shared_ptr to std::bind
        // would cause Socket to hold a shared_ptr<AgentChannel>, creating a
        // circular reference between Socket and AgentChannel.
        // This would prevent either from being freed --> memory leak
        weak_ptr<AgentChannel> ch = rrChannel;
        switch (socket->type()) {
            case Endpoint::Type::UDP:
                socket->setOnAsyncNewPacketCB(
                    std::bind(&RpcDispatch::onAsyncNewUdpPacket, this, ch,
                              require_addr));
                break;
            case Endpoint::Type::TCP:
                socket->setOnAsyncNewPacketCB(
                    std::bind(&RpcDispatch::onAsyncNewTcpPacket, this, ch));
                break;
        }
        socket->setWaiting(true);

        return;
    }
}

void
RpcDispatch::pollCall(const std::shared_ptr<AgentChannel>& rrChannel,
                         const std::shared_ptr<const Request>&,
                         const std::shared_ptr<Response>&)
{
}

void
RpcDispatch::selectCall(const std::shared_ptr<AgentChannel>& rrChannel,
                           const std::shared_ptr<const Request>&,
                           const std::shared_ptr<Response>&)
{
}

void
RpcDispatch::getpeernameCall(
    const std::shared_ptr<AgentChannel>& rrChannel,
    const std::shared_ptr<const Request>& req,
    const std::shared_ptr<Response>& resp)
{
    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();
    sockaddr_in sockname;

    {
        memset(&sockname, 0, sizeof(sockname));
        sockname.sin_family = AF_INET;
        sockname.sin_addr = pcb->faddr;
        sockname.sin_port = pcb->fport;
    }

    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_getpeernamecall();
        callRet->set_ret(0);
        // callRet->set_sockanme(&sockname, sizeof(struct sockaddr_in)); //
        // <--
        // use
        // after free
        callRet->set_peername(
            std::string{ (char*)&sockname, sizeof(struct sockaddr_in) });
    }
}

void
RpcDispatch::getsocknameCall(
    const std::shared_ptr<AgentChannel>& rrChannel,
    const std::shared_ptr<const Request>& req,
    const std::shared_ptr<Response>& resp)
{
    const auto& socket = rrChannel->socket();
    const auto& pcb = socket->pcb();
    sockaddr_in sockname;

    {
        memset(&sockname, 0, sizeof(sockname));
        sockname.sin_family = AF_INET;
        sockname.sin_addr = pcb->laddr;
        sockname.sin_port = pcb->lport;
    }

    {
        resp->set_retcode(Response::RetCode::Response_RetCode_OK);

        auto* callRet = resp->mutable_getsocknamecall();
        callRet->set_ret(0);
        // callRet->set_sockanme(&sockname, sizeof(struct sockaddr_in)); //
        // <--
        // use
        // after free
        callRet->set_sockanme(
            std::string{ (char*)&sockname, sizeof(struct sockaddr_in) });
    }
}

void
RpcDispatch::getsockoptCall(const std::shared_ptr<AgentChannel>& rrChannel,
                               const std::shared_ptr<const Request>&,
                               const std::shared_ptr<Response>& resp)
{
    /*
     * The only option the interceptor currently queries is SOL_SOCKET /
     * SO_ERROR.  The daemon clears errors once they have been reported
     * via a connect or recv response, so we always return 0 here.
     */
    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto* callRet = resp->mutable_getsockoptcall();
    callRet->set_ret(0);
}

void
RpcDispatch::setsockoptCall(const std::shared_ptr<AgentChannel>& rrChannel,
                               const std::shared_ptr<const Request>&,
                               const std::shared_ptr<Response>&)
{
    // Seems that we don't have to implement this API
}

void
RpcDispatch::fcntlCall(const std::shared_ptr<AgentChannel>& rrChannel,
                          const std::shared_ptr<const Request>&,
                          const std::shared_ptr<Response>& resp)
{
    /*
     * F_GETFD / F_SETFD / F_GETFL / F_SETFL are handled locally by
     * the interceptor using the nonblocking flag recorded at socket()
     * creation time.  The daemon simply acknowledges the call.
     */
    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto* callRet = resp->mutable_fcntlcall();
    callRet->set_ret(0);
}

void
RpcDispatch::atstartAction(const shared_ptr<AgentChannel>& rrChannel,
                              const shared_ptr<const Request>& req,
                              const shared_ptr<Response>& resp)
{
    const auto& call = req->atstartaction();
    const std::string& progname = call.progname();
    pid_t pid = req->pid();

    /*  1. Save peer information */

    rrChannel->setPeerName(progname);
    rrChannel->setPeerPid(pid);

    /*  2. Return information to the Interceptor */

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);

    auto atstart = resp->mutable_atstartaction();
    atstart->set_startfd(4096);
    atstart->set_count(1024);
}

void
RpcDispatch::onAsyncNewUdpPacket(
    const weak_ptr<AgentChannel>& rrChannel_weak, bool require_addr)
{
    shared_ptr<AgentChannel> rrChannel = rrChannel_weak.lock();
    if (!rrChannel)
        return;

    const auto& so = rrChannel->socket();

    shared_ptr<FrameBuf> skbuf_head;
    sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof(sockaddr_in));

    // take packet
    so->takeFromRecvQ(peeraddr, skbuf_head);

    // prepare Response
    auto resp = make_shared<Response>();
    auto* callRet = resp->mutable_recvfromcall();
    std::string* buf = callRet->mutable_buf();

    drain_skbuf_chain(skbuf_head, buf);

    // addr
    if (require_addr) {
        callRet->set_addr(std::string{ (char*)&peeraddr, sizeof(sockaddr_in) });
    }

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);
    callRet->set_ret(buf->length());
    rrChannel->write(resp);
}

void
RpcDispatch::onAsyncNewTcpPacket(
    const weak_ptr<AgentChannel>& rrChannel_weak)
{
    shared_ptr<AgentChannel> rrChannel = rrChannel_weak.lock();
    if (!rrChannel)
        return;

    const auto& so = rrChannel->socket();

    shared_ptr<FrameBuf> skbuf_head;
    sockaddr_in peeraddr; // just a placeholder

    // take packet
    so->takeFromRecvQ(peeraddr, skbuf_head);

    // prepare Response
    auto resp = make_shared<Response>();
    auto* callRet = resp->mutable_recvfromcall();
    std::string* buf = callRet->mutable_buf();

    drain_skbuf_chain(skbuf_head, buf);

    resp->set_retcode(Response::RetCode::Response_RetCode_OK);
    callRet->set_ret(buf->length());
    rrChannel->write(resp);
}
