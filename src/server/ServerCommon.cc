#include "ServerCommon.h"

#include <iostream>

void fail(boost::beast::error_code ec, char const * what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

StringSP toStringSP(const std::string & s)
{
    return std::make_shared<const std::string>(s);
}
