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
