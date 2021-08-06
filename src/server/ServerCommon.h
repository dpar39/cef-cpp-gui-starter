#pragma once
#include "boost/beast/core/error.hpp"

void fail(boost::beast::error_code ec, char const * what);
