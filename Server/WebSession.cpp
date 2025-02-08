#include "WebSession.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include "Digest.hpp"  

WebSocketSession::WebSocketSession(tcp::socket&& socket)
    : m_socket(std::move(socket)) {}

void WebSocketSession::doAccept(http::request<http::string_body> req) {
    std::cout << "WebSocket created." << std::endl;

    if (!authenticate(req)) {
        sendUnauthorized();
        return;
    }

    m_socket.async_accept(req,
        beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this()));
}

bool WebSocketSession::authenticate(const http::request<http::string_body>& req) {
    const auto& authHeader = req[http::field::authorization];
    if (authHeader.empty()) {
        return false;  
    }

    std::string authValue = authHeader;
    if (!boost::starts_with(authValue, "Digest ")) {
        return false; 
    }

    std::map<std::string, std::string> authParams;
    std::string authData = authValue.substr(7);
    std::stringstream ss(authData);
    std::string keyValuePair;
    while (std::getline(ss, keyValuePair, ',')) {
        size_t pos = keyValuePair.find('=');
        if (pos != std::string::npos) {
            std::string key = keyValuePair.substr(0, pos);
            std::string value = keyValuePair.substr(pos + 1);
            boost::trim(key);
            boost::trim(value);
            if (value.front() == '"') value.erase(0, 1);
            if (value.back() == '"') value.pop_back();
            authParams[key] = value;
        }
    }

    std::string username = authParams["username"];
    std::string realm = authParams["realm"];
    std::string uri = authParams["uri"];
    std::string response = authParams["response"];
    std::string method = req.method_string();

    if (username.empty() || realm.empty() || uri.empty() || response.empty()) {
        return false;
    }

    std::string expectedResponse = Digest::GenerateDigest(username, realm, uri, method);
    return response == expectedResponse;
}

void WebSocketSession::sendUnauthorized() {
    std::string unauthorizedMsg = "HTTP/1.1 401 Unauthorized\r\n"
        "WWW-Authenticate: Digest realm=\"audioserver\", qop=\"auth\", nonce=\"dummy_nonce\", opaque=\"dummy_opaque\"\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 10\r\n\r\n"
        "Unauthorized";

    m_socket.async_write(asio::buffer(unauthorizedMsg),
        beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
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
    std::cout << "Message recived: " << message << std::endl;

    std::istringstream iss(message);
    std::string command, username, password;
    std::getline(iss, command, ':');

    if (command == "login") {
        std::getline(iss, username, ':');
        std::getline(iss, password);

        bool valid = false;
        std::vector<std::string> files;
        std::string userDirectory;

        if (isValidUser(username, password, userDirectory)) {
            valid = true;

            if (std::filesystem::exists(userDirectory)) {
                for (const auto& entry : std::filesystem::directory_iterator(userDirectory)) {
                    if (entry.is_regular_file()) {
                        files.push_back(entry.path().filename().string());
                    }
                }
            }
        }

        if (valid) {
            std::ostringstream oss;
            oss << "{\"type\":\"login\",\"success\":true,\"files\":[";
            for (size_t i = 0; i < files.size(); i++) {
                oss << "\"" << files[i] << "\"";
                if (i < files.size() - 1)
                    oss << ",";
            }
            oss << "]}";

            sendMessage(oss.str());
        }
        else {
            sendMessage("{\"type\":\"login\",\"success\":false,\"error\":\"Wrong username or password\"}");
        }
    }
    else {
        sendMessage("{\"error\":\"Invalid command\"}");
    }

    doRead();
}

void WebSocketSession::sendMessage(const std::string& msg) {
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

bool WebSocketSession::isValidUser(const std::string& username, const std::string& password, std::string& userDirectory) {
    std::ifstream file("clients.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open clients.txt" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string folderAddress, storedHash;
        std::getline(iss, folderAddress, ' ');
        std::getline(iss, storedHash);

        std::string loginData = Digest::calculateMD5(username + ":/audioserver:" + password);

        if (loginData == storedHash) {
            userDirectory = "./storage/" + folderAddress;
            return true;
        }
    }

    return false;
}
