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

#ifndef VELOXD_INTERFACE_TUNIFACE_H
#define VELOXD_INTERFACE_TUNIFACE_H

#include <memory>

#include "PhysIface.h"

class FrameBuf;

class TunIface : public PhysIface
{
public:
    TunIface() = default;
    virtual ~TunIface() override;

    virtual void init() override;
    virtual void rx(std::shared_ptr<FrameBuf> skbuf) override;
    virtual void tx(const std::shared_ptr<FrameBuf>& skbuf) override;
    virtual void close() override;

private:
    void allocateTun();
    void setIfUp();
    void setIfAddr();

    char* _devname = nullptr;
    int _fd = 0;
};

#endif // VELOXD_INTERFACE_TUNIFACE_H
