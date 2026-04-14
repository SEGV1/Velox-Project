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

#ifndef VELOXD_INTERFACE_H
#define VELOXD_INTERFACE_H

#include <memory>

#include "veloxd/base/Singleton.h"

class FrameBuf;

class Interface : public Singleton<Interface>
{
    friend class Singleton<Interface>;

public:
    void init();
    void start();
    void stop();

    // For Ip layer use
    void tx(const std::shared_ptr<FrameBuf>& skbuf);

private:
    Interface();
    ~Interface();

    struct Impl; // pimpl forward declaration
    std::unique_ptr<Impl> _pImpl;
};

#endif // VELOXD_INTERFACE_H
