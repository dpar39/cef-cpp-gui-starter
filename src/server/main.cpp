#include "Server.h"
#include <cstdlib>
#include <filesystem>
#include <string>

std::string resolvePath(const std::string & relPath)
{
    namespace fs = std::filesystem;
    auto baseDir = fs::current_path();
    while (baseDir.has_parent_path())
    {
        auto combinePath = baseDir / relPath;
        if (exists(combinePath))
            return combinePath.string();

        const auto parentPath = baseDir.parent_path();
        if (parentPath == baseDir)
            break;
        baseDir = parentPath;
    }
    return {};
}

int main(int argc, char ** argv)
{
    const auto docRoot = resolvePath("gui");
    const auto portEnv = std::getenv("PORT");
    uint16_t port = 8100;
    if (portEnv != nullptr)
    {
        port = static_cast<uint16_t>(std::stoi(portEnv));
    }
    auto s = std::make_shared<Server>(docRoot, "127.0.0.1", port);
    s->run();
    return 0;
}
