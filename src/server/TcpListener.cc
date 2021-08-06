#include "TcpListener.h"

#include "HttpSession.h"
#include "ServerCommon.h"
#include <boost/asio/strand.hpp>

TcpListener::TcpListener(net::io_context & ioc, tcp::endpoint endPoint, std::string docRoot, OnWebsocketUpgrade cb)
: _ioc(ioc)
, _acceptor(net::make_strand(ioc))
, _docRoot(std::move(docRoot))
, _cb(std::move(cb))
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

uint16_t TcpListener::port() const
{
    const auto le = _acceptor.local_endpoint();
    return le.port();
}

void TcpListener::run()
{
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    net::dispatch(_acceptor.get_executor(), beast::bind_front_handler(&TcpListener::doAccept, this->shared_from_this()));
}

void TcpListener::doAccept()
{
    // The new connection gets its own strand
    _acceptor.async_accept(net::make_strand(_ioc),
                           beast::bind_front_handler(&TcpListener::onAccept, shared_from_this()));
}

void TcpListener::onAccept(beast::error_code ec, tcp::socket socket)
{
    if (ec)
    {
        fail(ec, "accept");
    }
    else
    {
        // Create the http session and run it
        std::make_shared<HttpSession>(std::move(socket), _docRoot, _cb)->run();
    }

    // Accept another connection
    doAccept();
}
