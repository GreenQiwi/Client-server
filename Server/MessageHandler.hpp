#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include "ServerStorage.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class MessageHandler {
public:
	static void handleRequest(beast::tcp_stream& stream, beast::flat_buffer& buffer);
};

