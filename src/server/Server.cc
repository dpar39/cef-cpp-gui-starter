//
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
#include <malloc.h>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Server.h"

#include "HttpSession.h"
#include "ServerCommon.h"
#include "common.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

// Accepts incoming connections and launches the sessions
FWD_DECL(Listener)
class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context & _ioc;
    tcp::acceptor _acceptor;
    std::string _docRoot;

public:
    Listener(net::io_context & ioc, tcp::endpoint endPoint, std::string docRoot)
    : _ioc(ioc)
    , _acceptor(net::make_strand(ioc))
    , _docRoot(std::move(docRoot))
    {
        beast::error_code ec;

        // Open the acceptor
        _acceptor.open(endPoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        _acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        _acceptor.bind(endPoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        _acceptor.listen(net::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    uint16_t port() const
    {
        const auto le = _acceptor.local_endpoint();
        return le.port();
    }

    // Start accepting incoming connections
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(_acceptor.get_executor(),
                      beast::bind_front_handler(&Listener::doAccept, this->shared_from_this()));
    }

private:
    void doAccept()
    {
        // The new connection gets its own strand
        _acceptor.async_accept(net::make_strand(_ioc),
                               beast::bind_front_handler(&Listener::onAccept, shared_from_this()));
    }

    void onAccept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the http session and run it
            _session = std::make_shared<HttpSession>(std::move(socket), _docRoot);
            _session->run();
        }

        // Accept another connection
        doAccept();
    }
    HttpSessionPtr _session;

public:
    HttpSessionPtr getHttpSession() const
    {
        return _session;
    }
};

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
    auto l = std::make_shared<Listener>(*_context, tcp::endpoint { address, port }, docRoot);
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
}
