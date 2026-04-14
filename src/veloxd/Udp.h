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

#ifndef VELOXD_UDP_H
#define VELOXD_UDP_H

#include "veloxd/base/Singleton.h"
#include <netinet/in.h>
#include <memory>

class FrameBuf;
class Session;

class Udp : public Singleton<Udp>
{
    friend class Singleton<Udp>;

public:
    void init();
    void start();
    void stop();

    std::shared_ptr<Session> newSession();
    void removeSession(const std::shared_ptr<const Session>&);

    // For IP Layer use
    void rx(const std::shared_ptr<FrameBuf>&);

private:
    Udp();
    ~Udp();

    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_UDP_H
