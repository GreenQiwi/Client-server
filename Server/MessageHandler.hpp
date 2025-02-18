#pragma once

#include "Session.hpp"

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class MessageHandler : public std::enable_shared_from_this<MessageHandler> {
public:
	MessageHandler(std::shared_ptr<asio::io_context> ioc);
	void Start();
	~MessageHandler() {
		std::cout << "MessageHandler destroyed" << std::endl;
	}
private:
	void acceptConnections();
	void handle(beast::error_code er, tcp::socket socket);

private:
	std::shared_ptr<asio::io_context> m_ioc;
	tcp::endpoint m_endpoint;
	tcp::acceptor m_acceptor;
	std::shared_ptr<http::request<http::string_body>> request;
	std::unordered_map<std::string, std::shared_ptr<Session>> m_sessions;
	std::mutex m_sessionsMutex;
};

