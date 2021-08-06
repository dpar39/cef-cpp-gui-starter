#pragma once

#include "common.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/optional/optional.hpp>

FWD_DECL(WebsocketSession)
FWD_DECL(HttpSession)
FWD_DECL(Queue)

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession>
{
    friend class Queue;

    beast::tcp_stream _stream;
    beast::flat_buffer _buffer;
    const std::string _docRoot;
    QueuePtr _queue;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

public:
    // Take ownership of the socket
    HttpSession(tcp::socket && socket, const std::string & docRoot);

    // Start the session
    void run();

private:
    void doRead();

    void onRead(beast::error_code ec, std::size_t bytesTransferred);

    void onWrite(bool close, boost::beast::error_code ec, std::size_t bytesTransferred);

    void doClose()
    {
        // Send a TCP shutdown
        boost::beast::error_code ec;
        _stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};
