#pragma once

#include "common.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>

FWD_DECL(WebsocketSession)

namespace boost::asio
{
class io_context;
}

namespace comms
{
class Message;
class Request;
class Response;
} // namespace comms

class StopSignalSet;

using ResponseCallback = std::function<void(const comms::Response &)>;

class ServerApp
{
public:
    ServerApp(const std::string & docRoot, const std::string & listeningAddress = "127.0.0.1", uint16_t port = 0);

    void runAsync();

    void run();

    void stop();

    virtual ~ServerApp();

    uint16_t getTcpPort() const;

    void handleRpcRequest(WebsocketSession * wss, const comms::Request & req, comms::Response & res);

    void callRpc(comms::Request * req, ResponseCallback callback, WebsocketSession * wss);

private:
    static void sendMessage(WebsocketSession * wss, comms::Message & res);

    void onWebsocketConnection(const WebsocketSessionPtr & ws);

    std::shared_ptr<boost::asio::io_context> _context;
    std::shared_ptr<StopSignalSet> _stopSignalSet;

    std::thread _runThread;
    std::uint16_t _port;
    std::string _address;

    std::vector<std::thread> _extraThreads;

    std::unordered_map<uint64_t, ResponseCallback> _pendingCallbacks;

    std::unordered_set<WebsocketSessionPtr> _websocketSessions;
    std::atomic<int> _msgId = 0;
};
