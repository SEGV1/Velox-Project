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

#include "veloxd/Tcp.h"

#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <list>

#include "veloxd/PseudoHdr.h"
#include "veloxd/Logging.h"
#include "veloxd/Session.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/arbiter/Endpoint.h"
#include "veloxd/tcp/TcpSession.h"

using std::unique_ptr;
using std::make_shared;
using std::shared_ptr;
using std::weak_ptr;

struct Tcp::Impl
{
    std::list<weak_ptr<Session>> pcbs;
};

Tcp::Tcp()
{
    _pImpl.reset(new Tcp::Impl);
}

Tcp::~Tcp() = default;

void
Tcp::init()
{
    // Nothing need to do
}

void
Tcp::start()
{
    // Nothing need to do
}

void
Tcp::stop()
{
}

shared_ptr<Session>
Tcp::newSession()
{
    const auto& pcb = make_shared<TcpSession>();
    _pImpl->pcbs.push_back(weak_ptr<Session>(pcb));

    return pcb;
}

void
Tcp::removeSession(const std::shared_ptr<const Session>& pcb)
{
    // TOOD
}

void
Tcp::rx(const shared_ptr<FrameBuf>& skbuf_head)
{
    /*  1. len & checksum */


    /*  2. demultiplex */

    PseudoHdr* iphdr_ovly = nullptr;
    TcpHdr* tcphdr = nullptr;

    struct in_addr faddr, laddr;
    __be16 fport, lport;

    iphdr_ovly = (PseudoHdr*)skbuf_head->network_hdr;
    tcphdr = (TcpHdr*)skbuf_head->transport_hdr;
    memcpy(&faddr, &iphdr_ovly->saddr, sizeof(struct in_addr));
    memcpy(&laddr, &iphdr_ovly->daddr, sizeof(struct in_addr));
    fport = tcphdr->source;
    lport = tcphdr->dest;

    shared_ptr<Session> pcb;
    pcb = Sessions::find(_pImpl->pcbs, faddr, fport, laddr, lport);
    if (!pcb) {
        // ICMP Type3 Destination Unreachable
        {
            VELOXD_LOG(info) << "No receiver, this TCP packet"
                                   " {source port = "
                                << __be16_to_cpu(fport)
                                << ", dest port = " << __be16_to_cpu(lport)
                                << "} will be droped ";
        }
        return;
    }

    pcb->recv(skbuf_head);
    {
        VELOXD_LOG(info) << "TCP Layer Delivered a TCP packet {sport = "
                            << ntohs(tcphdr->source)
                            << ", dport = " << ntohs(tcphdr->dest) << "}";
    }
}
