#pragma once

#include "ServerCommon.h"
#include "common.h"

#include <string>

#include <boost/beast/core/tcp_stream.hpp>

using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
namespace beast = boost::beast; // from <boost/beast.hpp>

FWD_DECL(Listener)

// Accepts incoming connections and launches the sessions
FWD_DECL(Listener)
class TcpListener : public std::enable_shared_from_this<TcpListener>
{
    net::io_context & _ioc;
    tcp::acceptor _acceptor;
    std::string _docRoot;
    OnWebsocketUpgrade _cb;

public:
    TcpListener(net::io_context & ioc, tcp::endpoint endPoint, std::string docRoot, OnWebsocketUpgrade cb);

    uint16_t port() const;

    // Start accepting incoming connections
    void run();

private:
    void doAccept();

    void onAccept(beast::error_code ec, tcp::socket socket);
};
