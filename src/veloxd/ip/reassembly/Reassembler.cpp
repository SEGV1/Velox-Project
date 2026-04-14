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

#include "veloxd/ip/Reassembler.h"

#include <algorithm>
#include <assert.h>
#include <list>

#include "veloxd/Logging.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/base/RawMutex.h"
#include "veloxd/ip/IpHdr.h"
#include "veloxd/ip/reassembly/FragGroup.h"

using std::shared_ptr;
using std::unique_ptr;

namespace {

// for timeout_cb use
std::list<FragGroup>* ctxList = nullptr;
RawMutex* lock;

void
timeout_cb(EV_P_ ev_timer* w, int /* unused */ revents)
{
    assert(ctxList != nullptr);

    // C-style programming

    // stop timer
    ev_timer_stop(EV_A_ w);

    MutexScope l(*lock);

    // Locate and remove from the list.
    //  Because a timeout and the arrival of all fragments can happen concurrently,
    //  by the time we reach here, ctx may have already been removed from the list,
    //  so we cannot assume ctx still exists in the list at this point
    FragGroup* ctx = (FragGroup*)(w - offsetof(FragGroup, timer));
    ctxList->remove_if([ctx](const FragGroup& cToPred) {
        if (&cToPred == ctx) {
            return true;
        } else {
            return false;
        }
    });
}
} // namespace {

struct Reassembler::Impl
{
    std::list<FragGroup> defragCtxList;
    RawMutex lock;
};

Reassembler::Reassembler()
{
    _pImpl.reset(new Reassembler::Impl);
    ctxList = &_pImpl->defragCtxList;
    lock = &_pImpl->lock;
}

Reassembler::~Reassembler() = default;

shared_ptr<FrameBuf>
Reassembler::defrag(const std::shared_ptr<FrameBuf>& skbuf)
{
    VELOXD_LOG(info) << "defragging";

    shared_ptr<FrameBuf> packet;
    FragGroup* ctx = nullptr;
    IpHdr* iphdr = (IpHdr*)skbuf->network_hdr;

    MutexScope l(_pImpl->lock);

    /*  1. Check whether a fragment group already exists for this packet */

    for (FragGroup& c : _pImpl->defragCtxList) {
        if (c.ip_id == iphdr->id && c.ip_protocol == iphdr->protocol &&
            c.ip_saddr == iphdr->saddr && c.ip_daddr == iphdr->daddr) {
            ctx = &c;
            break;
        }
    }

    /*  2. If not, create a new fragment group ctx */

    if (!ctx) {
        VELOXD_LOG(debug) << "Add one FragGroup to FragGroupList";
        _pImpl->defragCtxList.emplace_back();
        ctx = &_pImpl->defragCtxList.back();

        ctx->ip_id = iphdr->id;
        ctx->ip_protocol = iphdr->protocol;
        ctx->ip_saddr = iphdr->saddr;
        ctx->ip_daddr = iphdr->daddr;

        struct ev_loop* loop = EV_DEFAULT;
        ev_timer_init(&ctx->timer, &timeout_cb, 60,
                      0); // 60 seconds as RFC 1122
        ev_timer_start(EV_A_ & ctx->timer);
    }

    /*  3. Insert into the fragment group */

    // clang-format off
    if (!ctx->first
        || ( __be16_to_cpu(((IpHdr*)ctx->first->network_hdr)->frag_off)
             & IP_OFF_MASK )
           > ( __be16_to_cpu(iphdr->frag_off) & IP_OFF_MASK))
    {
        skbuf->next = ctx->first;
        ctx->first = skbuf;
    }
    else
    {
        shared_ptr<FrameBuf> prev;
        shared_ptr<FrameBuf> curr = ctx->first;

        int ours_off = iphdr->frag_off & IP_OFF_MASK;
        while (curr &&
               ours_off > (__be16_to_cpu(((IpHdr*)curr->network_hdr)->frag_off)
                           & IP_OFF_MASK)) {
            prev = curr;
            curr = curr->next;
        }

        prev->next = skbuf;
        skbuf->next = curr;
    }
    // clang-format on

    /*  4. Are all fragments in the group present?
     */

    shared_ptr<FrameBuf> curr = ctx->first;
    iphdr = nullptr;
    __le16 frag_off;
    int expectedOff = 0;
    __le16 payloadlen = 0;

    // Check whether the fragment offsets are contiguous from the first to the last fragment
    for (curr = ctx->first; curr; curr = curr->next) {
        iphdr = (IpHdr*)curr->network_hdr;
        int currOff = (__be16_to_cpu((iphdr->frag_off)) & IP_OFF_MASK) * 8;
        if (currOff != expectedOff)
            goto packet_not_ready;

        expectedOff = currOff + __be16_to_cpu(iphdr->tot_len) - iphdr->ihl * 4;
    }

    // Check whether the MF (More Fragments) bit of the last fragment is 0
    for (curr = ctx->first; curr->next; curr = curr->next)
        ;
    iphdr = (IpHdr*)curr->network_hdr;
    frag_off = __be16_to_cpu(iphdr->frag_off);
    if ((frag_off & IP_MF_MASK) != 0) {
        goto packet_not_ready;
    }
    payloadlen = (frag_off & IP_OFF_MASK) * 8 + __be16_to_cpu(iphdr->tot_len) -
                 iphdr->ihl * 4;

    {
        VELOXD_LOG(info) << "All IP fragments arrived, reassemblying";
    }

    /*  5. Reassemble the fragment group, then delete the reassembly context */

    // Adjust the first fragment
    packet = ctx->first;
    iphdr = (IpHdr*)packet->network_hdr;
    iphdr->tot_len = __cpu_to_be16(payloadlen + iphdr->ihl * 4);

    // Adjust subsequent fragments
    for (curr = ctx->first->next; curr; curr = curr->next) {
        iphdr = (IpHdr*)curr->network_hdr;
        int iphdrlen = iphdr->ihl;
        curr->network_hdr = nullptr;
        curr->user_payload_begin = curr->rawBytes + iphdrlen;
    }

    {
        VELOXD_LOG(info)
            << "Reassemblying finished, IP packet {payload length = "
            << payloadlen << "}";
    }

    ctxList->remove_if([ctx](const FragGroup& cToPred) {
        if (&cToPred == ctx) {
            VELOXD_LOG(debug)
                << "Remove one FragGroup from FragGroupList";
            return true;
        } else {
            return false;
        }
    });

packet_not_ready:

    VELOXD_LOG(debug) << "FragGroupList.size = "
                         << _pImpl->defragCtxList.size();

    return packet;
}
