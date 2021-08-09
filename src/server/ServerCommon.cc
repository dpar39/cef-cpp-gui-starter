#include "ServerCommon.h"

#include <filesystem>
#include <iostream>

void fail(boost::beast::error_code ec, char const * what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

StringSP toStringSP(std::string && s)
{
    return std::make_shared<const std::string>(s);
}

StringSP toStringSP(const std::string & s)
{
    return std::make_shared<const std::string>(s);
}

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
