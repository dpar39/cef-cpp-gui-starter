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

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Server.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http; // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio; // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

// Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos)
            return beast::string_view {};
        return path.substr(pos);
    }();
    if (iequals(ext, ".htm"))
        return "text/html";
    if (iequals(ext, ".html"))
        return "text/html";
    if (iequals(ext, ".php"))
        return "text/html";
    if (iequals(ext, ".css"))
        return "text/css";
    if (iequals(ext, ".txt"))
        return "text/plain";
    if (iequals(ext, ".js"))
        return "application/javascript";
    if (iequals(ext, ".json"))
        return "application/json";
    if (iequals(ext, ".xml"))
        return "application/xml";
    if (iequals(ext, ".swf"))
        return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))
        return "video/x-flv";
    if (iequals(ext, ".png"))
        return "image/png";
    if (iequals(ext, ".jpe"))
        return "image/jpeg";
    if (iequals(ext, ".jpeg"))
        return "image/jpeg";
    if (iequals(ext, ".jpg"))
        return "image/jpeg";
    if (iequals(ext, ".gif"))
        return "image/gif";
    if (iequals(ext, ".bmp"))
        return "image/bmp";
    if (iequals(ext, ".ico"))
        return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff"))
        return "image/tiff";
    if (iequals(ext, ".tif"))
        return "image/tiff";
    if (iequals(ext, ".svg"))
        return "image/svg+xml";
    if (iequals(ext, ".svgz"))
        return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path)
{
    if (base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto & c : result)
        if (c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <class Body, class Allocator, class Send>
void handle_request(beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>> && req, Send && send)
{
    // Returns a bad request response
    auto const bad_request = [&req](beast::string_view why) {
        http::response<http::string_body> res { http::status::bad_request, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found = [&req](beast::string_view target) {
        http::response<http::string_body> res { http::status::not_found, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error = [&req](beast::string_view what) {
        http::response<http::string_body> res { http::status::internal_server_error, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get && req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if (ec == beast::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if (ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res { http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res { std::piecewise_construct,
                                          std::make_tuple(std::move(body)),
                                          std::make_tuple(http::status::ok, req.version()) };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const * what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
    websocket::stream<beast::tcp_stream> _ws;
    beast::flat_buffer _buffer;

public:
    // Take ownership of the socket
    explicit WebsocketSession(tcp::socket && socket)
    : _ws(std::move(socket))
    {
    }

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

private:
    void onAccept(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        // Read a message
        doRead();
    }

    void doRead()
    {
        // Read a message into our buffer
        _ws.async_read(_buffer, beast::bind_front_handler(&WebsocketSession::onRead, shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        // This indicates that the websocket_session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
            fail(ec, "read");

        // Echo the message
        _ws.text(_ws.got_text());
        _ws.async_write(_buffer.data(), beast::bind_front_handler(&WebsocketSession::onWrite, shared_from_this()));
    }

    void onWrite(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Clear the buffer
        _buffer.consume(_buffer.size());

        // Do another read
        doRead();
    }
};

//------------------------------------------------------------------------------

// Handles an HTTP server connection
class HttpSession : public std::enable_shared_from_this<HttpSession>
{
    // This queue is used for HTTP pipelining.
    class Queue
    {
        enum
        {
            // Maximum number of responses we will queue
            limit = 8
        };

        // The type-erased, saved work item
        struct Work
        {
            virtual ~Work() = default;
            virtual void operator()() = 0;
        };

        HttpSession & _self;
        std::vector<std::unique_ptr<Work>> _items;

    public:
        explicit Queue(HttpSession & self)
        : _self(self)
        {
            static_assert(limit > 0, "queue limit must be positive");
            _items.reserve(limit);
        }

        // Returns `true` if we have reached the queue limit
        bool isFull() const
        {
            return _items.size() >= limit;
        }

        // Called when a message finishes sending
        // Returns `true` if the caller should initiate a read
        bool onWrite()
        {
            BOOST_ASSERT(!_items.empty());
            auto const was_full = isFull();
            _items.erase(_items.begin());
            if (!_items.empty())
                (*_items.front())();
            return was_full;
        }

        // Called by the HTTP handler to send a response.
        template <bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields> && msg)
        {
            // This holds a work item
            struct WorkImpl : Work
            {
                HttpSession & _self;
                http::message<isRequest, Body, Fields> _msg;

                WorkImpl(HttpSession & self, http::message<isRequest, Body, Fields> && msg)
                : _self(self)
                , _msg(std::move(msg))
                {
                }

                void operator()() override
                {
                    http::async_write(_self._stream,
                                      _msg,
                                      beast::bind_front_handler(&HttpSession::onWrite,
                                                                _self.shared_from_this(),
                                                                _msg.need_eof()));
                }
            };

            // Allocate and store the work
            _items.push_back(boost::make_unique<WorkImpl>(_self, std::move(msg)));

            // If there was no previous work, start this one
            if (_items.size() == 1)
                (*_items.front())();
        }
    };

    beast::tcp_stream _stream;
    beast::flat_buffer _buffer;
    const std::string _docRoot;
    Queue _queue;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

public:
    // Take ownership of the socket
    HttpSession(tcp::socket && socket, const std::string & docRoot)
    : _stream(std::move(socket))
    , _docRoot(docRoot)
    , _queue(*this)
    {
    }

    // Start the session
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(_stream.get_executor(), beast::bind_front_handler(&HttpSession::doRead, this->shared_from_this()));
    }

private:
    void doRead()
    {
        // Construct a new parser for each message
        parser_.emplace();

        // Apply a reasonable limit to the allowed size
        // of the body in bytes to prevent abuse.
        parser_->body_limit(1000000);

        // Set the timeout.
        _stream.expires_after(std::chrono::seconds(30));

        // Read a request using the parser-oriented interface
        http::async_read(_stream,
                         _buffer,
                         *parser_,
                         beast::bind_front_handler(&HttpSession::onRead, shared_from_this()));
    }

    void onRead(beast::error_code ec, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        // This means they closed the connection
        if (ec == http::error::end_of_stream)
            return doClose();

        if (ec)
            return fail(ec, "read");

        // See if it is a WebSocket Upgrade
        if (websocket::is_upgrade(parser_->get()))
        {
            // Create a websocket session, transferring ownership
            // of both the socket and the HTTP request.
            std::make_shared<WebsocketSession>(_stream.release_socket())->doAccept(parser_->release());
            return;
        }

        // Send the response
        handle_request(_docRoot, parser_->release(), _queue);

        // If we aren't at the queue limit, try to pipeline another request
        if (!_queue.isFull())
            doRead();
    }

    void onWrite(bool close, beast::error_code ec, std::size_t bytesTransferred)
    {
        boost::ignore_unused(bytesTransferred);

        if (ec)
            return fail(ec, "write");

        if (close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return doClose();
        }

        // Inform the queue that a write completed
        if (_queue.onWrite())
        {
            // Read another request
            doRead();
        }
    }

    void doClose()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        _stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
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
            std::make_shared<HttpSession>(std::move(socket), _docRoot)->run();
        }

        // Accept another connection
        doAccept();
    }
};

Server::Server(std::string docRoot, const std::string & listeningAddress, uint16_t port)
{
    const auto address = net::ip::make_address(listeningAddress.c_str());
    _context = std::make_shared<boost::asio::io_context>(1);

    // Create and launch a listening port
    auto l = std::make_shared<Listener>(*_context, tcp::endpoint { address, port }, docRoot);
    _port = l->port();
    l->run();
    _runThread = std::thread([&]() { _context->run(); });
}

Server::~Server()
{
    stop();
}

uint16_t Server::getTcpPort() const
{
    return _port;
}

void Server::stop()
{
    if (!_context)
        return;

    _context->stop();

    if (_runThread.joinable())
        _runThread.join();
    _context = nullptr;
}
