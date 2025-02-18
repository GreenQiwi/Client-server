#pragma once

#include <boost/beast/websocket/rfc6455.hpp>

#include "Digest.hpp"
#include "ServerStorage.hpp"
#include "WebSession.hpp"

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket&& socket);
    void Run();

private:
    beast::flat_buffer m_buffer;
    boost::optional<http::request_parser<http::string_body>> m_parser;
    tcp::socket m_socket;
    std::string m_ha1;
    std::string m_nonce;
    std::shared_ptr<http::response<http::string_body>> m_responce;

private:
    void doRead();
    void onRead(beast::error_code er, std::size_t s);

    void sendUnauthorized(http::request<http::string_body>& request);
    void handleWebClientRequest(http::request<http::string_body>& request);
    void sendFile(http::request<http::string_body>& request);

    bool validateToken(const std::string& token);
    std::string findUsername(const std::string& authHeader);

    void onWrite(beast::error_code er, size_t s);
};
