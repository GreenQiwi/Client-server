#pragma once

#include "Authentication.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Connection  { 
public:
    Connection(const std::string& host, const std::string& port);
    http::response<http::string_body> UploadFile(const std::string& filename, const std::string& target, const std::string& contentType, const std::string& digestResponse, std::string id);

private:
    asio::io_context m_ioc;
    std::string m_host;
    std::string m_port;

};
