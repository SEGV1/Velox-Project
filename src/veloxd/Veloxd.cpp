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

#include "Veloxd.h"

#include <bits/exception.h>
#include <cstdlib>
#include <iostream>

#include "veloxd/CfgParser.h"
#include "veloxd/Interface.h"
#include "veloxd/Ip.h"
#include "veloxd/Logging.h"
#include "veloxd/Arbiter.h"
#include "veloxd/Tcp.h"
#include "veloxd/Udp.h"

using std::cout;
using std::exception;

struct Veloxd::Impl
{
    bool running = false;
};

Veloxd::Veloxd()
{
    _pImpl.reset(new Veloxd::Impl);
}

