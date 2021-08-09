#include "ServerApp.h"
#include "ServerCommon.h"

#include <cstdlib>
#include <filesystem>
#include <string>

class ServerApp1 : public ServerApp
{
};

int main(int argc, char ** argv)
{
    const auto docRoot = resolvePath("gui");
    const auto portEnv = std::getenv("PORT");
    uint16_t port = 8100;
    if (portEnv != nullptr)
    {
        port = static_cast<uint16_t>(std::stoi(portEnv));
    }
    auto s = std::make_shared<ServerApp>(docRoot, "127.0.0.1", port);

    s->run();

    return 0;
}
