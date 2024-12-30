#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <iostream>
#include "ServerStorage.hpp"

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class MessageHandler : public std::enable_shared_from_this<MessageHandler> {
public:
	MessageHandler(tcp::acceptor acceptor);
	void Start();
	//static void handleRequest(beast::tcp_stream& stream, beast::flat_buffer& buffer);

private:
	void acceptConnections();
	void readRequest(tcp::socket socket);

private:
	tcp::acceptor m_acceptor;
	beast::flat_buffer m_buffer;
	 
	asio::thread_pool m_threadpool	;
};

