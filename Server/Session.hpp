#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include "Digest.hpp"
#include "ServerStorage.hpp"
#include <fstream>
#include <iostream>

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket&& socket);
    ~Session();
    void Run();

    //beast::flat_buffer GetBuffer() { return m_buffer; }
    //boost::optional<http::request_parser<http::string_body>> GetParser() { return m_parser; }
    //tcp::socket GetSocket() { return std::move(m_socket); }

private:
    beast::flat_buffer m_buffer;
    boost::optional<http::request_parser<http::string_body>> m_parser;
    tcp::socket m_socket;

private:
    void doRead();
    void onRead(beast::error_code er, std::size_t);
    void onWrite(beast::error_code write_er, std::size_t);
    void doClose();
};