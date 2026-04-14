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

Veloxd::~Veloxd() = default;

void
Veloxd::init(int argc, char* argv[])
{
    readConf(argc, argv);

    initSignalActions();
    initLogging();

    Interface::get()->init();
    Ip::get()->init();
    Tcp::get()->init();
    Udp::get()->init();
    Arbiter::get()->init();
}

void
Veloxd::run()
{
    _pImpl->running = true;

    Interface::get()->start();
    Ip::get()->start();
    Tcp::get()->start();
    Udp::get()->start();
    Arbiter::get()->start();

    pthread_exit(NULL);
}

void
Veloxd::stop()
{
    Interface::get()->stop();
    Ip::get()->stop();
    Tcp::get()->stop();
    Udp::get()->stop();
    Arbiter::get()->stop();

    _pImpl->running = false;
}

void
Veloxd::exit(enum exit_status e)
{
    if (_pImpl->running) {
        this->stop();
    }

    switch (e) {
        case exit_status::SUCCESS:
            ::exit(EXIT_SUCCESS);
        default:
            ::exit(EXIT_FAILURE);
    };
}

void
Veloxd::readConf(int argc, char* argv[])
{
    try {
        CfgParser reader;
        reader.readCmdLine(argc, argv);
        reader.readConfFile();
    } catch (const exception& e) {
        cout << "Error in Command line syntax"
             << "\n";
        cout << "  " << e.what() << "\n";
        this->exit(exit_status::FAILURE);
    }
}

void
Veloxd::initSignalActions()
{
}

void
Veloxd::initLogging()
{
    try {
        /*  1. global */
        LoggingInitializer i;
        i.run();

        /*  2. this thread */
        LoggingThreadInitializer ti;
        ti.run();
    } catch (const exception& e) {
        cout << "Error when starting logging facility"
             << "\n";
        cout << "  " << e.what() << "\n";
        this->exit(exit_status::FAILURE);
    }
}
