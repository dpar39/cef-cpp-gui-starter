#include "WebsocketSession.h"

#include "ServerCommon.h"

namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>

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
        return;

    if (ec)
        fail(ec, "read");

    if (onReadCallback)
        onReadCallback(_readBuffer);

    _writeBuffer = _readBuffer;
    doRead();

    // Echo the message
    _ws.text(_ws.got_text());
    _ws.async_write(_writeBuffer.data(), beast::bind_front_handler(&WebsocketSession::onWrite, shared_from_this()));
}

void WebsocketSession::onWrite(beast::error_code ec, std::size_t bytesTransferred)
{
    boost::ignore_unused(bytesTransferred);

    if (ec)
        return fail(ec, "write");

    // Clear the buffer
    _readBuffer.consume(_readBuffer.size());

    // Do another read
    doRead();
}
