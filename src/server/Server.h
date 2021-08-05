#pragma once

#include <cstdint>
#include <memory>
#include <stdint.h>
#include <string>
#include <thread>
#include <functional>

namespace boost::asio
{
class io_context;
}

class CrowApp;

class ReturnValue
{
    
};

struct Parameter
{
    std::string str;
    double doubleValue {0};
};

using MessageCallback = std::function<std::string(Parameter)>;

class Server
{

public:
    Server(std::string docRoot, const std::string & listeningAddress = "127.0.0.1", uint16_t port = 8120);

    void stop();

    ~Server();

    uint16_t getTcpPort() const;

    void registerMessage(const std::string & messageName, MessageCallback cb);

private:
    std::shared_ptr<boost::asio::io_context> _context;
    std::shared_ptr<CrowApp> _crowApp;

    std::thread _runThread;
    std::uint16_t _port;
};