#pragma once

#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include "Digest.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(tcp::socket&& socket);
    void doAccept(http::request<http::string_body> req);
    ~WebSocketSession();
private:
    websocket::stream<tcp::socket> m_socket;
    beast::flat_buffer m_buffer;
    std::string m_message;
    std::string m_userDirectory;

private:
    void onAccept(beast::error_code ec);
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytesTransferred);
    void sendMessage(const std::string& msg);
    void onWrite(beast::error_code ec, std::size_t bytesTransferred);
    void doClose();
    bool getUserFiles(const std::string& username, std::vector<std::string>& files);
    void sendFile(const std::string& filename);
    void sendFileList(const std::vector<std::string>& files);
    bool getUserDirectory(const std::string& username);
    std::string encodeBase64(const std::vector<char>& input);
    void deleteFile(const std::string& fileName);
    std::map<std::string, std::time_t> ReadAssociations(const std::string& file);
    void WriteAssociations(const std::map<std::string, std::time_t>& associations, const std::string& file);
};

