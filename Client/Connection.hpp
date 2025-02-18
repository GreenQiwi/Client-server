#pragma once

#include "Authentication.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

#define HTTP_VERSION 11

class Connection {
public:
    Connection(const std::string& host, const std::string& port);
    http::response<http::string_body> UploadFile(const std::string& filename, const std::string& target, 
        const std::string& contentType, std::string& authToken, const std::string& ha1, const std::string& authHeader, const std::string& username);

private:
    asio::io_context m_ioc;
    tcp::resolver m_resolver;
    std::string m_host;
    std::string m_port;

};
