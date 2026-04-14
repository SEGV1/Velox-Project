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

#include "veloxd/tcp/TcpSession.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

#include "veloxd/Ip.h"
#include "veloxd/PseudoHdr.h"
#include "veloxd/Logging.h"
#include "veloxd/Arbiter.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/base/Csum16.h"
#include "veloxd/arbiter/EventReactor.h"
#include "veloxd/arbiter/Endpoint.h"
#include "veloxd/tcp/TcpHdr.h"
#include "veloxd/tcp/TcpTimer.h"

using std::shared_ptr;
using std::make_shared;
using std::string;

namespace {

__be16
nextAvailPort()
{
    static uint16_t port = 1024;
    ++port;
    return __cpu_to_be16(port);
}

} // namespace {

TcpSession::TcpSession()
    : Session()
{
    pipe2(estab_notify_pipe, O_NONBLOCK);
    sChannel.reset(new IoHandle(estab_notify_pipe[0]));
    sChannel->enableReading();
    sChannel->setOnReadCB([this]() {
        char readbuf[8];
        while (::read(this->estab_notify_pipe[0], readbuf, 8) != -1)
            ;

        if (onConnEstabCB)
            onConnEstabCB();
    });
}

__be16
TcpSession::nextAvailLocalPort()
{
    return nextAvailPort();
}

int
TcpSession::connect(const struct in_addr& faddr, __be16 fport)
{
    // Super class first
    Session::connect(faddr, fport);

    // Only send the first SYN here,
    std::string empty;
    this->send(empty);

    return 0;
}

void
TcpSession::setOnConnEstabCB(const std::function<void()>& f)
{
    this->onConnEstabCB = f;
}

IoHandle*
TcpSession::getConnEstabNotifyChannel()
{
    return this->sChannel.get();
}

void
TcpSession::disconnect()
{
    // Four-way handshake (connection teardown)
}

int
TcpSession::send(const std::string& buf)
{
    // Super class first
    Session::send(buf);

    shared_ptr<FrameBuf> skbuf_head = this->socketBufferOfTcpTempl();
    TcpHdr* tcphdr = (TcpHdr*)skbuf_head->transport_hdr;

    /*  1. Different TCP states send different types of TCP packets */

    switch (this->conn_state) {
        case TcpState::TCP_STATE__CLOSED:
            this->send_unack = this->send_next = this->iss;
            tcphdr->seq = htonl(send_next), ++send_next;
            tcphdr->syn = 1;

            this->conn_state = TCP_STATE__SYN_SENT; //
            break;
        case TcpState::TCP_STATE__ESTABLISHED:
            tcphdr->seq = htonl(send_next);
            send_next += buf.length();
            tcphdr->ack = 1;
            tcphdr->ack_seq = htonl(recv_next);
            break;
        default:
            // - Client states SYN_SENT, FIN_WAIT_1, FIN_WAIT_2
            //   only need to be handled when processing asynchronously arriving packets
            // - Client TIME_WAIT state is handled by a timer
            // - Other states are for the server side, which we don't consider (since we don't implement a server)
            return 0;
    }

    this->prepareBeforeIpTx(skbuf_head, buf);
    Ip::get()->tx(skbuf_head, buf);

    // Pretend no error occurred
    return buf.length();
}

void
TcpSession::recv(const std::shared_ptr<FrameBuf>& skbuf)
{
    TcpHdr* tcphdr = (TcpHdr*)skbuf->transport_hdr;

    switch (this->conn_state) {
        case TcpState::TCP_STATE__SYN_SENT:
            assert(tcphdr->syn && tcphdr->ack);

            /*  1. recv SYN & ACK */
            this->irs = ntohl(tcphdr->seq);
            this->recv_next = irs + 1;
            ++this->send_unack;
            this->conn_state = TCP_STATE__ESTABLISHED;

            /*  2. sent ACK */

            this->ack2peer(recv_next);

            /*  3. callback */

            if (onConnEstabCB) {
                onConnEstabCB();
            }

            break;
        case TcpState::TCP_STATE__ESTABLISHED:
            if (tcphdr->ack) {
                uint32_t k = ntohl(tcphdr->ack_seq);
                if (k > this->send_unack) {
                    this->send_unack = ntohl(tcphdr->ack_seq);
                }
            }
            skbuf->user_payload_begin = skbuf->transport_hdr + tcphdr->doff * 4;
            if (skbuf->user_payload_end - skbuf->user_payload_begin) {
                // Save to Socket buffer
                struct sockaddr_in peer_addr;
                shared_ptr<Endpoint> so = this->socket().lock();
                so->putToRecvQ(peer_addr, skbuf);

                // Send ACK
                if (tcphdr->seq != this->recv_next) {
                    VELOXD_LOG(warning) << "Out-of-order TCP segment...";
                }
                this->recv_next = ntohl(tcphdr->seq) + skbuf->user_payload_end -
                                  skbuf->user_payload_begin;
                this->ack2peer(this->recv_next);
            } else {
                // do nothing
            }
            break;
        default:
            // - Client states SYN_SENT, FIN_WAIT_1, FIN_WAIT_2
            //   only need to be handled when processing asynchronously arriving packets
            // - Client TIME_WAIT state is handled by a timer
            // - Other states are for the server side, which we don't consider (since we don't implement a server)
            return;
    }
}

shared_ptr<FrameBuf>
TcpSession::socketBufferOfTcpTempl()
{
    /*  1. FrameBuf */

    /*  First FrameBuf in the FrameBuf chain
     *   -  The remaining FrameBufs in the chain are built during IP-layer
     *      fragmentation; only the first FrameBuf needs to be built here
     */

    const auto& skbuf_head = make_shared<FrameBuf>();
    // Pointers to each Header within the FrameBuf
    {
        bzero(skbuf_head->rawBytes, 120); // IP 60 + TCP 60

        skbuf_head->hdrs_begin =
            skbuf_head->rawBytes + 40; // 40 reserved for IP options
        skbuf_head->network_hdr = skbuf_head->hdrs_begin;
        skbuf_head->transport_hdr = skbuf_head->network_hdr + 20;
        skbuf_head->hdrs_end = skbuf_head->transport_hdr + 20;
    }

    /*  2. Generic TCP and IP headers */

    TcpHdr* tcphdr = (TcpHdr*)skbuf_head->transport_hdr;
    {
        tcphdr->source = this->lport;
        tcphdr->dest = this->fport;
        tcphdr->doff = 20 / 4;
        tcphdr->window = htons(4096);
    }

    return skbuf_head;
}

void
TcpSession::prepareBeforeIpTx(const std::shared_ptr<FrameBuf>& skbuf_head,
                          const string& buf)
{
    PseudoHdr* ipovly = (PseudoHdr*)skbuf_head->network_hdr;
    TcpHdr* tcphdr = (TcpHdr*)skbuf_head->transport_hdr;

    // IP Header set by the transport layer that is "relevant to TCP checksum computation"
    {
        ipovly->protocol = 6; // 6 stands for TCP
        ipovly->protocol_len = htons(buf.length() + 20);
        memcpy(&ipovly->saddr, &this->laddr, sizeof(__be32));
        memcpy(&ipovly->daddr, &this->faddr, sizeof(__be32));
    }

    // TCP Header
    {
        // uint32_t hdr_checksum = Csum16::checksum(
        //     skbuf_head->network_hdr,
        //     tcphdr->doff * 4 + 20 /* IP Header length*/, 0);
        // if (buf.length()) {
        //     tcphdr->check = Csum16::checksum(
        //         buf.c_str(), buf.length(), hdr_checksum);
        // } else {
        //     tcphdr->check = hdr_checksum;
        // }

        uint32_t sum = 0;
        if (buf.length()) {
            int count = buf.length();

            uint16_t* ptr = (uint16_t*)buf.c_str();
            while (count > 1) {
                /*  This is the inner loop */
                sum += *ptr++;
                count -= 2;
            }

            /*  Add left-over byte, if any */
            if (count > 0)
                sum += *(uint8_t*)ptr;
        }
        tcphdr->check = Csum16::checksum(
            skbuf_head->network_hdr,
            tcphdr->doff * 4 + 20 /* IP Header length*/, sum);
    }

    // IP Header set by the transport layer that is "irrelevant to TCP checksum computation"
    {
        ipovly->ttl = 64;
        ipovly->tos = 0;
        ipovly->len = htons(buf.length() + 40);
        ipovly->protocol_len = 0; // don't forget this (iphdr->checksum)
    }

    // logging
    {
        char src_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        char dest_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        inet_ntop(AF_INET, &ipovly->saddr, src_ipaddr_p, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ipovly->daddr, dest_ipaddr_p, INET_ADDRSTRLEN);

        VELOXD_LOG(info)
            << "Packet {source = " << src_ipaddr_p << ":"
            << ntohs(tcphdr->source) << ", dest = " << dest_ipaddr_p << ":"
            << ntohs(tcphdr->dest) << "}, passing to IP Layer...";
    }
}

void
TcpSession::ack2peer(tcp_seq value)
{
    shared_ptr<FrameBuf> skbuf_head = this->socketBufferOfTcpTempl();
    TcpHdr* tcphdr = (TcpHdr*)skbuf_head->transport_hdr;

    tcphdr->ack = 1;
    tcphdr->ack_seq = htonl(value);
    tcphdr->seq = htonl(this->send_next);

    prepareBeforeIpTx(skbuf_head);
    string empty;
    Ip::get()->tx(skbuf_head, empty);
}
