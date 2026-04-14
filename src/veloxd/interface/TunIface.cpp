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

#include "TunIface.h"

#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>

#include <exception>
#include <memory>

#include "veloxd/Conf.h"
#include "veloxd/Logging.h"
#include "veloxd/FrameBuf.h"
#include "veloxd/base/ShellOp.h"
#include "veloxd/Veloxd.h"

using std::exception;
using std::shared_ptr;

TunIface::~TunIface()
{
    if (_devname) {
        delete[] _devname;
    }
}

// http://backreference.org/2010/03/26/tuntap-interface-tutorial/
// https://www.kernel.org/doc/Documentation/networking/tuntap.txt
void
TunIface::init()
{
    PhysIface::init();

    allocateTun();
    setIfUp();
    setIfAddr();
}

void
TunIface::rx(shared_ptr<FrameBuf> skbuf)
{
    int rc = 0;
    int errno_ = 0;

    rc = ::read(_fd, skbuf->rawBytes, skbuf->maxRawBytesSize);
    if (rc == -1) {
        errno_ = errno;
        VELOXD_LOG(fatal) << "Error when reading from TUN device: "
                             << std::string(strerror(errno_));
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }

    skbuf->network_hdr = skbuf->rawBytes;
    skbuf->hdrs_begin = skbuf->rawBytes;
    skbuf->user_payload_end = skbuf->hdrs_begin + rc;

    VELOXD_LOG(info) << "TUN device received a packet";
}

void
TunIface::tx(const shared_ptr<FrameBuf>& skbuf_head)
{
    struct iovec buffers[32]; // 32 is enough, maybe...
    int idx = 0;
    int n2written = 0;
    int nwritten = 0;

    shared_ptr<FrameBuf> curr_skbuf;
    for (curr_skbuf = skbuf_head; curr_skbuf; curr_skbuf = curr_skbuf->next) {
        assert(curr_skbuf->hdrs_begin && curr_skbuf->hdrs_end &&
               curr_skbuf->user_payload_begin && curr_skbuf->user_payload_end);

        buffers[idx].iov_base = curr_skbuf->hdrs_begin;
        buffers[idx].iov_len = curr_skbuf->hdrs_end - curr_skbuf->hdrs_begin;
        n2written += buffers[idx].iov_len;
        ++idx;

        buffers[idx].iov_base = curr_skbuf->user_payload_begin;
        buffers[idx].iov_len =
            curr_skbuf->user_payload_end - curr_skbuf->user_payload_begin;
        n2written += buffers[idx].iov_len;
        ++idx;
    }

    nwritten = writev(_fd, buffers, idx);
    {
        if (nwritten == -1) {
            int errno_ = errno;
            VELOXD_LOG(error) << "Error when Writing to TunIface: "
                                 << std::string(strerror(errno_));
        }

        VELOXD_LOG(info)
            << "TunIface transmited an packet {n2written = " << n2written
            << ", nwritten = " << nwritten << "}";
    }
}

void
TunIface::close()
{
    if (_fd != 0) {
        ::close(_fd);
    }
}

void
TunIface::allocateTun()
{
    int fd;
    struct ifreq ifr;
    const char* clonedev = "/dev/net/tun";

    /*  1. open file descriptor */

    if ((fd = open(clonedev, O_RDWR)) < 0) {
        int errno_ = errno;
        VELOXD_LOG(fatal)
            << "Failed to open TUN device: " << std::string(strerror(errno_));
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }

    /*  2. allocate TUN device
     *
     * Flags:
     *     IFF_TUN   - TUN device (no Ethernet headers)
     *     IFF_TAP   - TAP device
     *     IFF_NO_PI - Do not offer us packet information
     * ipv4 / ipv6
     */

    memset(&ifr, 0, sizeof(struct ifreq));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (!Conf::get()->tundev.name.empty()) {
        strcpy(_devname, Conf::get()->tundev.name.c_str());
    }

    int rc = ioctl(fd, TUNSETIFF, (void*)&ifr);
    if (rc < 0) {
        int errno_ = errno;
        VELOXD_LOG(fatal) << "Failed to allocate TUN device: "
                             << std::string(strerror(errno_));
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }

    /*  3. copy name */

    _devname = new char[IFNAMSIZ];
    strcpy(_devname, ifr.ifr_name);
    _fd = fd;
}

void
TunIface::setIfUp()
{
    try {
        ShellOp r("ip link set dev %s up", _devname);
        r.run();
    } catch (std::exception& e) {
        VELOXD_LOG(fatal)
            << "Failed to set TUN device up: " << std::string(e.what());
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }
}

void
TunIface::setIfAddr()
{
    try {
        ShellOp r("ip addr add dev %s local %s", _devname,
                    (Conf::get()->tundev.addr_cidr).c_str());
        r.run();
    } catch (std::exception& e) {
        VELOXD_LOG(fatal)
            << "Failed to set TUN device address: " << std::string(e.what());
        Veloxd::get()->stop();
        Veloxd::get()->exit(Veloxd::exit_status::FAILURE);
    }
}
