#pragma once

#include <cstdint>
#include <memory>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>

namespace boost::asio
{
class io_context;
}

class Server
{

public:
    Server(const std::string & staticDir, const std::string & listeningAddress = "127.0.0.1", uint16_t port = 0);
    void stop();

    ~Server();

    uint16_t getTcpPort() const
    {
        return _port;
    }

private:
    std::shared_ptr<boost::asio::io_context> _context;
    std::thread _runThread;
    std::uint16_t _port;
};