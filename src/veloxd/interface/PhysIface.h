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

#ifndef VELOXD_INTERFACE_PHYSIFACE_H
#define VELOXD_INTERFACE_PHYSIFACE_H

#include <functional>
#include <memory>

class FrameBuf;

class PhysIface
{
public:
    PhysIface() = default;
    virtual ~PhysIface();
    // Well, non-copyable & non-moveable
    PhysIface(const PhysIface&) = delete;
    PhysIface(PhysIface&&) = delete;
    PhysIface& operator=(const PhysIface*) = delete;
    PhysIface& operator=(PhysIface*) = delete;

    virtual void init();
    virtual void rx(std::shared_ptr<FrameBuf> skbuf);
    virtual void tx(const std::shared_ptr<FrameBuf>& skbuf);
    virtual void close();
};

#endif // VELOXD_INTERFACE_PHYSIFACE_H
