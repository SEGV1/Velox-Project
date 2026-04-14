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

#include "AgentChannel.h"

#include <google/protobuf/util/json_util.h>
#include <string>
#include <sys/un.h>
#include <unistd.h>

#include "veloxd/Logging.h"
#include "veloxd/Arbiter.h"
#include "veloxd/arbiter/IoHandle.h"
#include "rpc/cpp_out/request.pb.h"
#include "rpc/cpp_out/response.pb.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::make_shared;
using std::string;
using std::vector;

class Connection;

struct AgentChannel::Impl
{
    /*  Data members */

    int fd;
    unique_ptr<IoHandle> sChannel; // For EventReactor use

    string peer; // Name of peer interceptor
    pid_t peerPid;

    shared_ptr<Endpoint> socket;

    /*  Member functions */

    // clang-format off
    std::function
     <
        shared_ptr<Response>
        (const shared_ptr<AgentChannel>& rrChannel
         , const shared_ptr<const Request>& req)
     > onNewRequestFunc;
    // clang-format on
    void write(const shared_ptr<Response>&);

    void onReadReady(const shared_ptr<AgentChannel>&);
    void onPeerWritingClose(const shared_ptr<AgentChannel>&);
};

AgentChannel::AgentChannel(int fd)
{
    {
        VELOXD_LOG(debug) << "New AgentChannel {fd = " << fd << "}";
    }

    _pImpl.reset(new AgentChannel::Impl);
    _pImpl->fd = fd;

    /*  1. IoHandle for EventReactor */

    auto& sChannel = _pImpl->sChannel;
    sChannel.reset(new IoHandle(fd));
    sChannel->enableReading();
    sChannel->setOnReadCB(
        [this]() { this->_pImpl->onReadReady(this->shared_from_this()); });
    sChannel->setOnCloseCB([this]() {
        this->_pImpl->onPeerWritingClose(this->shared_from_this());
    });
}

AgentChannel::~AgentChannel()
{
    _pImpl->sChannel->unregisterSelf();
    ::close(_pImpl->fd);

    {
        VELOXD_LOG(debug) << "Destroy AgentChannel";
    }
}

void
AgentChannel::setOnNewRequestCB(const std::function<shared_ptr<Response>(
                                 const shared_ptr<AgentChannel>&,
                                 const shared_ptr<const Request>& req)>& f)
{
    assert(f);
    _pImpl->onNewRequestFunc = f;
}

void
AgentChannel::write(const std::shared_ptr<Response>& resp)
{
    _pImpl->write(resp);
}

IoHandle*
AgentChannel::getIoHandle()
{
    return _pImpl->sChannel.get();
}

void
AgentChannel::setPeerName(const std::string& p)
{
    _pImpl->peer = p;
}

void
AgentChannel::setPeerPid(pid_t p)
{
    _pImpl->peerPid = p;
}

const std::string&
AgentChannel::peerName()
{
    return _pImpl->peer;
}

shared_ptr<Endpoint>
AgentChannel::socket()
{
    return _pImpl->socket;
}

void
AgentChannel::setSocket(const std::shared_ptr<Endpoint>& s)
{
    _pImpl->socket = s;
}

void
AgentChannel::Impl::onReadReady(const shared_ptr<AgentChannel>& rrChannel)
{
    // One rdbuf per thread (for reading data from the UNIX socket)
    // (This may feel slightly counter-intuitive for OOP, but greatly reduces memory usage)
    static __thread char rdbuf[RPC_MESSAGE_MAX_SIZE];

    // Prevent rrChannel from being destroyed while this function is running (it actually would be)
    shared_ptr<AgentChannel> holdit{ rrChannel };

    shared_ptr<Request> req = make_shared<Request>();
    shared_ptr<Response> resp;

    /*  1. read the message */

    memset(rdbuf, 0, sizeof(rdbuf));
    int nread = ::read(fd, rdbuf, sizeof(rdbuf));
    if (nread == -1) {
        int errno_ = errno;

        if (errno_ == EAGAIN || errno_ == EWOULDBLOCK) {
            return;
        }

        // VELOXD_LOG(error) << "::read " << string(strerror(errno_))
        //                      << ". This channel will be closed";
        // sChannel->unregisterSelf();
        // return;
    } else if (nread == 0) {
        VELOXD_LOG(info) << "::read EOF"
                             << ". This channel will be closed";
        Arbiter::get()->removeRRChannel(rrChannel);
        return;
    } else if (nread == RPC_MESSAGE_MAX_SIZE) {
        VELOXD_LOG(warning) << "::read call get RPC_MESSAGE_MAX_SIZE bytes";
    }

    /*  2. parse the message */

    req->ParseFromArray(rdbuf, nread);

    {
        // For DEBUG
        string jsonStr;
        google::protobuf::util::MessageToJsonString(*req, &jsonStr);
        VELOXD_LOG(debug)
            << "Recv " << nread << " bytes, request message: " << jsonStr;
    }

    /*  3. handle the request message, generate response message */

    assert(onNewRequestFunc);
    resp = onNewRequestFunc(rrChannel, req);

    /*  4. write the message */

    write(resp);
}

void
AgentChannel::Impl::onPeerWritingClose(const shared_ptr<AgentChannel>& who)
{
    Arbiter::get()->removeRRChannel(who);
}

void
AgentChannel::Impl::write(const std::shared_ptr<Response>& resp)
{
    static __thread char wrbuf[RPC_MESSAGE_MAX_SIZE];

    {
        // For DEBUG
        string jsonStr;
        google::protobuf::util::MessageToJsonString(*resp.get(), &jsonStr);
        VELOXD_LOG(debug) << "Send response message: " << jsonStr;
    }

    assert(resp->IsInitialized());
    int needwr = resp->ByteSize();
    assert(needwr <= RPC_MESSAGE_MAX_SIZE);
    // if (needwr > RPC_MESSAGE_MAX_SIZE) {
    //     VELOXD_LOG(error)
    //         << "Serialized message size exceeds RPC_MESSAGE_MAX_SIZE";
    // }
    resp->SerializeToArray(wrbuf, RPC_MESSAGE_MAX_SIZE);
    int nwritten = ::write(fd, wrbuf, needwr);
    if (nwritten == -1) {
        int errno_ = errno;
        VELOXD_LOG(error) << "::write " << string(strerror(errno_))
                             << ". This channel will be closed";
        sChannel->unregisterSelf();
        return;
    } else if (nwritten < needwr) {
        VELOXD_LOG(error)
            << "::write <<" << nwritten << " bytes, which less than" << needwr
            << " bytes to write";
    }
}
