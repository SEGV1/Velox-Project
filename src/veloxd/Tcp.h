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

#ifndef VELOXD_TCP_H
#define VELOXD_TCP_H

#include "veloxd/base/Singleton.h"
#include <memory>

class FrameBuf;
class Session;

class Tcp : public Singleton<Tcp>
{
    friend class Singleton<Tcp>;

public:
    void init();
    void start();
    void stop();

    std::shared_ptr<Session> newSession();
    void removeSession(const std::shared_ptr<const Session>&);

    void rx(const std::shared_ptr<FrameBuf>&);
    void tx();

private:
    Tcp();
    ~Tcp();

    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_TCP_H
