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

#include "veloxd/Udp.h"

#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <list>

// #include "veloxd/base/BoundedDeque.h"
#include "veloxd/PseudoHdr.h"
#include "veloxd/Logging.h"
#include "veloxd/Session.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/udp/UdpHdr.h"
#include "veloxd/udp/UdpSession.h"

using std::unique_ptr;
using std::make_shared;
using std::weak_ptr;
using std::shared_ptr;

struct Udp::Impl
{
    // Udp::rx
    std::list<weak_ptr<Session>> pcbs;
};

Udp::Udp()
{
    _pImpl.reset(new Udp::Impl);
}

Udp::~Udp() = default;

void
Udp::init()
{
    // Nothing need to do
}

void
Udp::start()
{
    // Nothing need to do
}

void
Udp::stop()
{
}

shared_ptr<Session>
Udp::newSession()
{
    const auto& pcb = make_shared<UdpSession>();
    const auto& weak_pcb = pcb;
    _pImpl->pcbs.push_back(weak_pcb);

    return pcb;
}

void
Udp::removeSession(const std::shared_ptr<const Session>& pcb)
{
}

void
Udp::rx(const shared_ptr<FrameBuf>& skbuf_head)
{
    /*  1. len & checksum */


    /*  2. demultiplex */

    PseudoHdr* iphdr_ovly = nullptr;
    UdpHdr* udphdr = nullptr;

    struct in_addr faddr, laddr;
    __be16 fport, lport;

    iphdr_ovly = (PseudoHdr*)skbuf_head->network_hdr;
    udphdr = (UdpHdr*)skbuf_head->transport_hdr;
    memcpy(&faddr, &iphdr_ovly->saddr, sizeof(struct in_addr));
    memcpy(&laddr, &iphdr_ovly->daddr, sizeof(struct in_addr));
    fport = udphdr->source;
    lport = udphdr->dest;

    shared_ptr<Session> pcb;
    pcb = Sessions::find(_pImpl->pcbs, faddr, fport, laddr, lport);
    if (!pcb) {
        {
            VELOXD_LOG(info) << "No receiver, this UDP packet"
                                   " {source port = "
                                << __be16_to_cpu(fport)
                                << ", dest port = " << __be16_to_cpu(lport)
                                << "} will be droped ";
        }
        return;
    }

    sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof(peeraddr));
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_addr = *(struct in_addr*)&iphdr_ovly->saddr;
    peeraddr.sin_port = udphdr->source;

    pcb->recv(peeraddr, skbuf_head);

    {
        VELOXD_LOG(info) << "UDP Layer Delivered an UDP packet {sport = "
                            << ntohs(udphdr->source)
                            << ", dport = " << ntohs(udphdr->dest) << "}";
    }
}
