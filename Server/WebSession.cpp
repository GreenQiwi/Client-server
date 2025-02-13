#include "WebSession.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <filesystem>

WebSocketSession::WebSocketSession(tcp::socket&& socket)
    : m_socket(std::move(socket)) {}

WebSocketSession::~WebSocketSession() { std::cout << "WebSession destroyed." << std::endl; }

void WebSocketSession::doAccept(http::request<http::string_body> req) {
    std::cout << "WebSocket created." << std::endl;

    m_socket.async_accept(req,
        beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this()));
}

void WebSocketSession::onAccept(beast::error_code ec) {
    if (ec) {
        std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
        return;
    }
    std::cout << "WebSocket connection accepted." << std::endl;
    doRead();
}

void WebSocketSession::doRead() {
    m_socket.async_read(m_buffer,
        beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t bytesTransferred) {
    boost::ignore_unused(bytesTransferred);
    if (ec == websocket::error::closed) {
        std::cout << "WebSocket connection closed." << std::endl;
        return;
    }
    if (ec) {
        std::cerr << "WebSocket read error: " << ec.message() << std::endl;
        return;
    }

    std::string message = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(m_buffer.size());
    std::cout << "Message received: " << message << std::endl;

    std::istringstream iss(message);
    std::string command;
    std::getline(iss, command, ':');

    if (command == "requestFiles") {
        std::string username;
        std::getline(iss, username);

        std::vector<std::string> files;

        if (getUserFiles(username, files)) {
            sendFileList(files);
        }
        else {
            sendMessage("{\"error\":\"User not found\"}");
        }
    }
    else if (command == "requestFile") {
        std::string username, fileName;
        std::getline(iss, username, ':');
        std::getline(iss, fileName);

        if (getUserDirectory(username)) {
            sendFile(fileName);
        }
        else {
            std::cout << "User directory not found " << m_userDirectory << std::endl;
            sendMessage("{\"error\":\"User directory not found\"}");
        }
    }
    else {
        sendMessage("{\"error\":\"Invalid command\"}");
    }

    doRead();
}

bool WebSocketSession::getUserFiles(const std::string& username, std::vector<std::string>& files) {
    std::ifstream file("clients.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open clients.txt" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string folderAddress, storedHash, storedUsername;
        std::getline(iss, folderAddress, ' ');
        std::getline(iss, storedHash, ' ');
        std::getline(iss, storedUsername);

        if (storedUsername == username) {
            m_userDirectory = "./storage/" + folderAddress;
            if (std::filesystem::exists(m_userDirectory)) {
                for (const auto& entry : std::filesystem::directory_iterator(m_userDirectory)) {
                    if (entry.is_regular_file()) {
                        files.push_back(entry.path().filename().string());
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool WebSocketSession::getUserDirectory(const std::string& username) {
    std::ifstream file("clients.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open clients.txt" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string folderAddress, storedHash, storedUsername;
        std::getline(iss, folderAddress, ' ');
        std::getline(iss, storedHash, ' ');
        std::getline(iss, storedUsername);

        if (storedUsername == username) {
            m_userDirectory = "./storage/" + folderAddress;
            return true;
        }
    }
    return false;
}

void WebSocketSession::sendFileList(const std::vector<std::string>& files) {
    std::ostringstream oss;
    oss << "{\"type\":\"fileList\",\"files\":[";
    for (size_t i = 0; i < files.size(); i++) {
        oss << "\"" << files[i] << "\"";
        if (i < files.size() - 1)
            oss << ",";
    }
    oss << "]}";
    sendMessage(oss.str());
}

void WebSocketSession::sendFile(const std::string& filename) {
    std::ifstream file(m_userDirectory + "/" + filename, std::ios::binary);
    if (!file.is_open()) {
        sendMessage("{\"error\":\"File not found\"}");
        return;
    }

    std::vector<char> fileData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    m_socket.binary(true);
    m_socket.async_write(asio::buffer(fileData),
        beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
}

void WebSocketSession::sendMessage(const std::string& msg) {
    if (!m_socket.is_open()) {
        std::cerr << "Attempted to send message on a closed socket." << std::endl;
        return;
    }

    m_socket.text(true);
    m_socket.async_write(asio::buffer(msg),
        beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t s) {
    if (ec) {
        std::cerr << "WebSocket write error: " << ec.message() << std::endl;
    }
}

void WebSocketSession::doClose() {
    beast::error_code ec;
    if (!ec) {
        m_socket.close(tcp::socket::shutdown_send, ec);
    }
    else {
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    }
    std::cout << "Session ended, socket closed." << std::endl;
}
