#include "WebsocketSession.h"

#include "ServerCommon.h"

namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;

WebsocketSession::WebsocketSession(tcp::socket && socket)
: _ws(std::move(socket))
{
}

void WebsocketSession::onAccept(beast::error_code ec)
{
    if (ec)
        return fail(ec, "accept");

    // Read a message
    doRead();
}

void WebsocketSession::doRead()
{
    // Read a message into our buffer
    _ws.async_read(_readBuffer, beast::bind_front_handler(&WebsocketSession::onRead, shared_from_this()));
}

void WebsocketSession::onRead(beast::error_code ec, std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    // This indicates that the websocket_session was closed
    if (ec == websocket::error::closed)
    {
        onMessage = nullptr;
        if (onClose)
            onClose();
        return;
    }

    if (ec)
    {
        fail(ec, "read");
        return;
    }

    if (onMessage)
        onMessage(_readBuffer);

    _readBuffer.consume(_readBuffer.size());
    doRead();
}

void WebsocketSession::send(const StringSP & ss)
{
    net::post(_ws.get_executor(), beast::bind_front_handler(&WebsocketSession::onSend, shared_from_this(), ss));
}

void WebsocketSession::onSend(const StringSP & ss)
{
    // Always add to queue
    _queue.push_back(ss);

    // Are we already writing?
    if (_queue.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    _ws.binary(true);
    const auto & msg = _queue.front();
    _ws.async_write(net::buffer(msg->data(), msg->size()),
                    beast::bind_front_handler(&WebsocketSession::onWrite, shared_from_this()));
}

void WebsocketSession::onWrite(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if (ec)
        return fail(ec, "write");

    // Remove the string from the queue
    _queue.pop_front();

    // Send the next message if any
    if (!_queue.empty())
    {
        const auto & msg = _queue.front();
        _ws.async_write(net::buffer(msg->data(), msg->size()),
                        beast::bind_front_handler(&WebsocketSession::onWrite, shared_from_this()));
    }
}
