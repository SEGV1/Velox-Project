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

#ifndef VELOXD_TCP_TCPSESSION_H
#define VELOXD_TCP_TCPSESSION_H

#include "veloxd/Session.h"
#include "veloxd/arbiter/IoHandle.h"
#include "veloxd/tcp/TcpHdr.h"

typedef uint32_t tcp_seq;

class TcpSession : public Session
{
public:
    // let compiler generate ctor/dctor
    TcpSession();

    // override functions
    virtual int connect(const struct in_addr& faddr, __be16 fport) override;
    virtual void setOnConnEstabCB(const std::function<void()>&) override;
    virtual IoHandle* getConnEstabNotifyChannel() override;
    virtual void disconnect() override;
    virtual int send(const std::string& buf) override;
    virtual void recv(const std::shared_ptr<FrameBuf>&) override;

    virtual __be16 nextAvailLocalPort() override;

private:
    std::shared_ptr<FrameBuf> socketBufferOfTcpTempl();
    void prepareBeforeIpTx(const std::shared_ptr<FrameBuf>&,
                           const std::string& buf = std::string());
    void ack2peer(tcp_seq);

private:
    constexpr static int msl = 30; // 30s

    short conn_state = TcpState::TCP_STATE__CLOSED;

    /* connect */
    int estab_notify_pipe[2];
    std::function<void()> onConnEstabCB;
    std::unique_ptr<IoHandle> sChannel;

    short send_flags = 0;

    /* send sequence */
    const static tcp_seq iss = 1;
    tcp_seq send_unack;
    tcp_seq send_next;
    tcp_seq peer_recv_adv;

    /* receive sequence */
    // tcp_seq rcv_wnd;
    tcp_seq irs;
    tcp_seq recv_next;

    /* congestion control */
};

#endif
