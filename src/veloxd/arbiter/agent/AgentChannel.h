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

#ifndef VELOXD_MUX_AGENTCHANNEL_H
#define VELOXD_MUX_AGENTCHANNEL_H

#include <functional>
#include <memory>
#include <vector>

class Endpoint;
class Request;
class Response;
class IoHandle;

class AgentChannel : public std::enable_shared_from_this<AgentChannel>
{
public:
    // AgentChannel owns the fd and is responsible for close(fd) when it is destroyed
    AgentChannel(int fd);
    ~AgentChannel();
    // Non-copyable, Non-moveable
    AgentChannel(const AgentChannel&) = delete;
    AgentChannel& operator=(const AgentChannel&) = delete;
    AgentChannel(AgentChannel&&) = delete;
    AgentChannel& operator=(AgentChannel&&) = delete;

    // When EventReactor notifies AgentChannel of a read event, AgentChannel
    // automatically calls the callback registered via onNewRequest to handle the Request.
    //
    // The entire automatic handling process runs in the EventReactor IO thread,
    // which requires the registered callback onNewRequest **must not block**.
    //
    // Hint: the callback onNewRequest can avoid blocking by using the approach of
    //  [ notifying the ProtocolControlBlock, so that ProtocolControlBlock can
    //    proactively write(Response) when resources become available ]
    //
    // Note: whether to pass shared_ptr<AgentChannel> as the first parameter of
    //  the function is a design decision. Here, it is not passed as the first
    //  parameter; instead, Arbiter captures it in the lambda and passes it to
    //  RpcDispatch.
    //
    // clang-format off
    void setOnNewRequestCB(
        const std::function
         <
           std::shared_ptr<Response>
           (const std::shared_ptr<AgentChannel>&
            , const std::shared_ptr<const Request>& req)
         > &
    );
    // clang-format on

    // Write a Response into AgentChannel (direct read/write, not the callback path)
    void write(const std::shared_ptr<Response>&);

    IoHandle* getIoHandle();

    // process information
    void setPeerName(const std::string&);
    const std::string& peerName();
    void setPeerPid(pid_t);

    // Connected below to Socket
    std::shared_ptr<Endpoint> socket();
    void setSocket(const std::shared_ptr<Endpoint>&);

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif
