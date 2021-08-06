#pragma once
#include "common.h"

#include <boost/beast/core/error.hpp>

#include <functional>

FWD_DECL(WebsocketSession);
using OnWebsocketUpgrade = std::function<void(const WebsocketSessionPtr &)>;

void fail(boost::beast::error_code ec, char const * what);

using StringSP = std::shared_ptr<const std::string>;

StringSP toStringSP(const std::string & s);
