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

#ifndef VELOXD_SESSION_H
#define VELOXD_SESSION_H

#include <asm/byteorder.h>
#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>

class Endpoint;
class FrameBuf;
class IoHandle;

class Session : public std::enable_shared_from_this<Session>
{
public:
    struct in_addr faddr;
    __be16 fport;
    struct in_addr laddr;
    __be16 lport;

public:
    Session();
    virtual ~Session();
    // Non-copyable, Non-moveable
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    int bind();
    virtual int connect(const struct in_addr& faddr, __be16 fport);
    virtual void setOnConnEstabCB(const std::function<void()>&);
    virtual IoHandle* getConnEstabNotifyChannel(); 
    virtual void disconnect();
    virtual int send(const std::string& buf);
    virtual int send(const struct in_addr& faddr, __be16 fport,
                     const std::string& buf);
    virtual void recv(const std::shared_ptr<FrameBuf>&);
    virtual void recv(struct sockaddr_in& peeraddr,
                      const std::shared_ptr<FrameBuf>&);
    virtual __be16 nextAvailLocalPort() = 0; // each protocol implements
    // void bind(struct in_addr laddr, __be16 lport);

    std::weak_ptr<Endpoint> socket();
    void setSocket(const std::weak_ptr<Endpoint>&);

private:
    std::weak_ptr<Endpoint> _socket;
};

class Sessions
{
public:
    template <typename Container>
    static std::shared_ptr<Session> find(Container c, struct in_addr faddr,
                                     __be16 fport, struct in_addr laddr,
                                     __be16 lport)
    {
        // clang-format off
        std::function< int
                      (const struct in_addr& ours, const struct in_addr& theirs)
                     > in_addr_match
             = [](const struct in_addr& ours, const struct in_addr& theirs)
               -> int
               {
                   uint32_t ours_ = *(uint32_t*)&ours;
                   uint32_t theirs_ = *(uint32_t*)&theirs;

                   if (ours_ == 0){
                     return 0;
                   } else if (ours_ == theirs_){
                     return 1;
                   } else {
                     return -1;
                   }
               };
        // clang-format on

        typename Container::const_iterator it;
        std::shared_ptr<Session> pcb;

        int best_score = -1;  // best match score seen so far
        for (it = c.cbegin(); it != c.cend(); ++it) {
            const std::shared_ptr<Session>& curr = (*it).lock();
            if (!curr) {
                continue;
            }

            int accum = 0;
            int addr_m = 0;  // separate var to avoid shadowing best_score

            {
                if (curr->lport != lport) {
                    continue;
                } else {
                    accum += 1;
                }

                if (curr->fport != 0) {
                    if (curr->fport == fport) {
                        accum += 1;
                    } else {
                        continue;
                    }
                } else {
                    //  accum += 0  (wildcard fport scores lower than exact)
                }

                if ((addr_m = in_addr_match(curr->faddr, faddr)) == -1) {
                    continue;
                }
                accum += addr_m;  // 0 (wildcard) or 1 (exact)

                if ((addr_m = in_addr_match(curr->laddr, laddr)) == -1) {
                    continue;
                }
                accum += addr_m;  // 0 (wildcard) or 1 (exact)
            }

            if (accum > best_score) {
                pcb = curr;
                best_score = accum;
            }
        }

        return pcb;
    }
};

#endif
