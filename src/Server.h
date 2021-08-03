#pragma once

#include <memory>
#include <string>
#include <vector>
#include <thread>

namespace boost::asio
{
class io_context;
}

class Server
{

public:
    Server(const std::string & listeningAddress, uint16_t port, const std::string & staticDir);
    void stop();

private:
    std::shared_ptr<boost::asio::io_context> _context;
    std::thread _runThread;
};