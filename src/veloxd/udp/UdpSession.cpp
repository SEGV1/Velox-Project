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

#include "veloxd/udp/UdpSession.h"

#include <arpa/inet.h>
#include <memory>

#include "veloxd/Ip.h"
#include "veloxd/PseudoHdr.h"
#include "veloxd/Logging.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/Udp.h"
#include "veloxd/arbiter/Endpoint.h"
#include "veloxd/udp/UdpHdr.h"

using std::shared_ptr;
using std::make_shared;

namespace {

__be16
nextAvailPort()
{
    static uint16_t port = 1024;
    ++port;
    return __cpu_to_be16(port);
}

} // namespace {

__be16
UdpSession::nextAvailLocalPort()
{
    return nextAvailPort();
}

int
UdpSession::send(const std::string& buf)
{
    // Super class first
    Session::send(buf);

    /*  First FrameBuf in the FrameBuf chain
     *   -  The remaining FrameBufs in the chain are built during IP-layer
     *      fragmentation; only the first FrameBuf needs to be built here
     */

    const auto& skbuf_head = make_shared<FrameBuf>();

    // Pointers to each Header within the FrameBuf
    {
        bzero(skbuf_head->rawBytes, 68);

        skbuf_head->hdrs_begin =
            skbuf_head->rawBytes + 40; // 40 reserved for IP options
        skbuf_head->network_hdr = skbuf_head->hdrs_begin;
        skbuf_head->transport_hdr = skbuf_head->network_hdr + 20;
        skbuf_head->hdrs_end = skbuf_head->hdrs_begin + 28;
    }

    PseudoHdr* ipovly = (PseudoHdr*)skbuf_head->network_hdr;
    UdpHdr* udphdr = (UdpHdr*)skbuf_head->transport_hdr;

    // IP Header set by the transport layer that is "relevant to UDP checksum computation"
    {
        ipovly->protocol = 17; // 17 stands for UDP
        ipovly->protocol_len = htons(buf.length() + 8);
        memcpy(&ipovly->saddr, &this->laddr, sizeof(__be32));
        memcpy(&ipovly->daddr, &this->faddr, sizeof(__be32));
    }

    // UDP Header
    {
        udphdr->source = this->lport;
        udphdr->dest = this->fport;
        udphdr->len = htons(buf.length() + 8);
        bzero(&udphdr->check, sizeof(__be16));
    }

    // IP Header set by the transport layer that is "irrelevant to UDP checksum computation"
    {
        ipovly->ttl = 64;
        ipovly->tos = 0;
        ipovly->len = htons(buf.length() + 28);
    }

    // logging
    {
        char src_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        char dest_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        inet_ntop(AF_INET, &ipovly->saddr, src_ipaddr_p, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ipovly->daddr, dest_ipaddr_p, INET_ADDRSTRLEN);

        VELOXD_LOG(info)
            << "Packet {source = " << src_ipaddr_p << ":"
            << ntohs(udphdr->source) << ", dest = " << dest_ipaddr_p << ":"
            << ntohs(udphdr->dest) << "}, passing to IP Layer...";
    }

    ipovly->protocol_len = 0; // don't forget this (iphdr->checksum)
    Ip::get()->tx(skbuf_head, buf);

    // Pretend no error occurred
    return buf.length();
}

int
UdpSession::send(const struct in_addr& faddr, __be16 fport, const std::string& buf)
{
    // Super class first
    Session::send(faddr, fport, buf);

    connect(faddr, fport);
    int ret = this->send(buf);
    disconnect();

    return ret;
}

void
UdpSession::recv(struct sockaddr_in& peeraddr,
             const std::shared_ptr<FrameBuf>& skbuf)
{
    // Super class first
    Session::recv(peeraddr, skbuf);

    skbuf->user_payload_begin = skbuf->transport_hdr + 8;

    shared_ptr<Endpoint> socket = this->socket().lock();
    socket->putToRecvQ(peeraddr, skbuf);
}
