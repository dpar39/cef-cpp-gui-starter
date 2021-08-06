﻿//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: Advanced server
//
//------------------------------------------------------------------------------

#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Server.h"

#include "HttpSession.h"
#include "ServerCommon.h"
#include "TcpListener.h"

namespace net = boost::asio; // from <boost/asio.hpp>

class StopSignalSet : public net::signal_set
{
public:
    StopSignalSet(const std::shared_ptr<boost::asio::io_context> & ioc)
    : net::signal_set(*ioc, SIGINT, SIGTERM)
    {
        async_wait([&](beast::error_code const &, int) {
            if (ioc && !ioc->stopped())
                ioc->stop();
        });
    }
};

Server::Server(std::string docRoot, const std::string & listeningAddress, uint16_t port)
{
    const auto address = net::ip::make_address(listeningAddress.c_str());
    _context = std::make_shared<boost::asio::io_context>(1);

    // Create and launch a listening port
    auto l = std::make_shared<TcpListener>(*_context,
                                           tcp::endpoint { address, port },
                                           docRoot,
                                           [this](const WebsocketSessionPtr & p) { onWebsocketConnection(p); });
    _port = l->port();
    _address = listeningAddress;
    l->run();
    _stopSignalSet = std::make_shared<StopSignalSet>(_context);
}

void Server::runAsync()
{
    _runThread = std::thread([&]() { run(); });
}

void Server::run()
{
    std::cout << "Server running on http://" << _address << ":" << _port << std::endl;

    const auto threads = 2;
    _extraThreads.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        _extraThreads.emplace_back([this] { _context->run(); });
    _context->run();
    std::cout << "Server stopped" << std::endl;
}

Server::~Server()
{
    stop();
}

uint16_t Server::getTcpPort() const
{
    return _port;
}

ResponsePtr Server::onInvoke(const RequestPtr & req)
{
    return nullptr;
}

void Server::stop()
{
    if (_context->stopped())
        return;
    _context->stop();
    if (_runThread.joinable())
        _runThread.join();
    for (auto & t : _extraThreads)
        if (t.joinable())
            t.join();
}

void Server::onWebsocketConnection(const WebsocketSessionPtr & ws)
{
    ws->onMessage = [ws](beast::flat_buffer & buffer) {
        std::string s = beast::buffers_to_string(buffer.data());
        std::cout << s << std::endl;
        ws->send(toStringSP(s + " back!!"));
    };

    // ws->send(toStringSP("Connection established"));
}
