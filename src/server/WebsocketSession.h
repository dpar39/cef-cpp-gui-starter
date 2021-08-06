#pragma once
#include "ServerCommon.h"

#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <deque>

FWD_DECL(WebSocketSession)

namespace beast = boost::beast; // from <boost/beast.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
namespace websocket = beast::websocket;
namespace http = beast::http; // from <boost/beast/http.hpp>

// Echoes back all received WebSocket messages
class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
    websocket::stream<beast::tcp_stream> _ws;
    beast::flat_buffer _readBuffer;
    std::deque<StringSP> _queue;

public:
    std::function<void(beast::flat_buffer &)> onMessage;

public:
    // Take ownership of the socket
    explicit WebsocketSession(tcp::socket && socket);

    // Start the asynchronous accept operation
    template <class Body, class Allocator>
    void doAccept(http::request<Body, http::basic_fields<Allocator>> req)
    {
        // Set suggested timeout settings for the websocket
        _ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        _ws.set_option(websocket::stream_base::decorator([](websocket::response_type & res) {
            res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " advanced-server");
        }));

        // Accept the websocket handshake
        _ws.async_accept(req, beast::bind_front_handler(&WebsocketSession::onAccept, shared_from_this()));
    }

    void send(const StringSP & ss);

private:
    void onSend(const StringSP & ss);

    void onAccept(beast::error_code ec);

    void doRead();

    void onRead(beast::error_code ec, std::size_t bytesTransferred);

    void onWrite(beast::error_code ec, std::size_t bytesTransferred);
};
