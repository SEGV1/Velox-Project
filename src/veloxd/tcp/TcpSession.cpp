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

#include "veloxd/tcp/TcpSession.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

#include "veloxd/Ip.h"
#include "veloxd/PseudoHdr.h"
#include "veloxd/Logging.h"
#include "veloxd/Arbiter.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/base/Csum16.h"
#include "veloxd/arbiter/EventReactor.h"
#include "veloxd/arbiter/Endpoint.h"
#include "veloxd/tcp/TcpHdr.h"
#include "veloxd/tcp/TcpTimer.h"

using std::shared_ptr;
using std::make_shared;
using std::string;

namespace {

__be16
nextAvailPort()
{
    static uint16_t port = 1024;
    ++port;
    return __cpu_to_be16(port);
}

} // namespace {

TcpSession::TcpSession()
    : Session()
{
    pipe2(estab_notify_pipe, O_NONBLOCK);
    sChannel.reset(new IoHandle(estab_notify_pipe[0]));
    sChannel->enableReading();
    sChannel->setOnReadCB([this]() {
