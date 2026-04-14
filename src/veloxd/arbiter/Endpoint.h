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

#ifndef VELOXD_MUX_ENDPOINT_H
#define VELOXD_MUX_ENDPOINT_H

#include <functional>
#include <memory>
#include <netinet/in.h>

class AgentChannel;
class FrameBuf;
class IoHandle;
class Session;

class Endpoint
{
public:
    enum Type
    {
        UDP,
        TCP,
    };

public:
    Endpoint(Type, bool nonblocking);
    ~Endpoint();
    // Non-copyable, Non-moveable
    Endpoint(const Endpoint&) = delete;
    Endpoint& operator=(const Endpoint&) = delete;
    Endpoint(Endpoint&&) = delete;
    Endpoint& operator=(Endpoint&&) = delete;

    enum Type type();
    bool nonblocking();

    void setWaiting(bool);
    void setOnAsyncNewPacketCB(std::function<void()>);
    IoHandle* getAsyncNewPacketNotifyChannel();

    void putToRecvQ(struct sockaddr_in&, const std::shared_ptr<FrameBuf>&);
    void takeFromRecvQ(struct sockaddr_in&, std::shared_ptr<FrameBuf>&);
    bool try_takeFromRecvQ(struct sockaddr_in&, std::shared_ptr<FrameBuf>&);

    // Connected above to AgentChannel (its lifecycle is managed by AgentChannel)
    std::weak_ptr<AgentChannel> reqRespChannel();
    void setAgentChannel(const std::shared_ptr<AgentChannel>&);

    // Connected below to Session (manages the lifecycle of Session)
    std::shared_ptr<Session> pcb();
    void setSession(const std::shared_ptr<Session>&);

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif
