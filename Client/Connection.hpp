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

class Connection {
public:
    Connection(const std::string& host, const std::string& port);
    void UploadFile(const std::string& filename, const std::string& target, const std::string& contentType, const std::string& login, const std::string& password);

private:
    std::string host;
    std::string port;
};