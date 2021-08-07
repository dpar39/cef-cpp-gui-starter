#pragma once

#include "HttpSession.h"
#include "WebsocketSession.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <thread>


namespace boost::asio
{
class io_context;
}

class StopSignalSet;

class Server
{
public:
    Server(std::string docRoot, const std::string & listeningAddress = "127.0.0.1", uint16_t port = 0);

    void runAsync();

    void run();

    void stop();
    void onWebsocketConnection(const WebsocketSessionPtr & ws);

    virtual ~Server();

    uint16_t getTcpPort() const;


private:
    std::shared_ptr<boost::asio::io_context> _context;
    std::shared_ptr<StopSignalSet> _stopSignalSet;

    std::thread _runThread;
    std::uint16_t _port;
    std::string _address;

    std::vector<std::thread> _extraThreads;
};
