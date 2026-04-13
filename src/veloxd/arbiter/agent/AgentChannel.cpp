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

#include "AgentChannel.h"

#include <google/protobuf/util/json_util.h>
#include <string>
#include <sys/un.h>
#include <unistd.h>

#include "veloxd/Logging.h"
#include "veloxd/Arbiter.h"
#include "veloxd/arbiter/IoHandle.h"
#include "rpc/cpp_out/request.pb.h"
#include "rpc/cpp_out/response.pb.h"
#include "rpc/rpc.h"

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::make_shared;
using std::string;
using std::vector;

class Connection;

struct AgentChannel::Impl
{
    /*  Data members */

    int fd;
    unique_ptr<IoHandle> sChannel; // For EventReactor use

    string peer; // Name of peer interceptor
    pid_t peerPid;
