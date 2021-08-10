
#include "ServerApp.h"

#include <boost/asio/signal_set.hpp>
#include <google/protobuf/util/json_util.h>

#include <iostream>
#include <memory>
#include <thread>

#include "WebsocketSession.h"

#include "HttpSession.h"
#include "ServerCommon.h"
#include "TcpListener.h"
#include "messages.pb.h"

namespace net = boost::asio; // from <boost/asio.hpp>
using namespace std;

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

ServerApp::ServerApp(const std::string & docRoot, const std::string & listeningAddress, uint16_t port)
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

void ServerApp::runAsync()
{
    _runThread = std::thread([&]() { run(); });
}

void ServerApp::run()
{
    std::cout << "Server running on http://" << _address << ":" << _port << std::endl;

    const auto threads = 2;
    _extraThreads.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        _extraThreads.emplace_back([this] { _context->run(); });
    _context->run();
    std::cout << "Server stopped" << std::endl;
}

ServerApp::~ServerApp()
{
    stop();
}

uint16_t ServerApp::getTcpPort() const
{
    return _port;
}

void ServerApp::stop()
{
    if (_runThread.joinable())
        _runThread.join();
    for (auto & t : _extraThreads)
        if (t.joinable())
            t.join();
    if (_context->stopped())
        return;
    _context->stop();
}

void ServerApp::onWebsocketConnection(const WebsocketSessionPtr & ws)
{
    ws->onMessage = [ws, this](beast::flat_buffer & buffer) {
        using namespace comms;
        Message msgIn;
        const auto & mutableBuffer = buffer.data();
        msgIn.ParseFromArray(mutableBuffer.data(), static_cast<int>(mutableBuffer.size()));

        if (false)
        {
            std::string json;
            google::protobuf::util::MessageToJsonString(msgIn, &json);
            std::cout << json << std::endl;
        }

        if (msgIn.has_request())
        {
            // RPC call to the server
            const auto & req = msgIn.request();
            Message msgOut;
            msgOut.set_id(msgIn.id());

            auto * res = new Response();
            msgOut.set_allocated_response(res);

            handleRpcRequest(ws.get(), req, *res);
            sendMessage(ws.get(), msgOut);
        }
        else if (msgIn.has_response())
        {
            // look for pending callbacks
            const auto it = _pendingCallbacks.find(msgIn.id());
            if (it != _pendingCallbacks.end())
            {
                throw std::runtime_error("Got a response, but no callback to handle it");
            }
            if (const auto & cb = it->second; cb != nullptr)
                cb(msgIn.response());
            _pendingCallbacks.erase(it);
        }
        else
        {
            throw std::runtime_error("A message needs to have either a request or a response!");
        }
    };

    ws->onClose = [ws, this]() {
        std::cout << "Websocket connection closed" << std::endl;
        _websocketSessions.erase(ws);
    };

    _websocketSessions.insert(ws);
}

void ServerApp::callRpc(comms::Request * req, ResponseCallback callback, WebsocketSession * wss)
{
    comms::Message msg;
    ++_msgId;
    msg.set_id(_msgId);
    msg.set_allocated_request(req);
    _pendingCallbacks.emplace(_msgId, std::move(callback));
}

void ServerApp::handleRpcRequest(WebsocketSession * wss, const comms::Request & req, comms::Response & res)
{
    if (req.has_echo())
    {
        auto echo = new comms::Echo();
        echo->set_text(req.echo().text());
        res.set_allocated_echo(echo);
        return;
    }

    if (req.has_listdir())
    {
        auto * ld = new comms::ListDirectoryRes();
        auto n = ld->add_nodes();
        n->set_isfile(true);
        n->set_size(4567);
        n->set_name("this-file.txt");
        res.set_allocated_listdir(ld);
        return;
    }
}

void ServerApp::sendMessage(WebsocketSession * wss, comms::Message & msg)
{
    string os;
    msg.SerializeToString(&os);
    wss->send(toStringSP(std::move(os)));
}
