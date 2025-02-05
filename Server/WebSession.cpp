#include "WebSession.hpp"
#include <filesystem>

WebSocketSession::WebSocketSession(tcp::socket&& socket)
    : m_socket(std::move(socket))
{}

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
    std::cout << "Message recived: " << message << std::endl;

    std::istringstream iss(message);
    std::string command, username, password;
    std::getline(iss, command, ':');

    if (command == "login") {
        std::getline(iss, username, ':');
        std::getline(iss, password);

        bool valid = false;
        std::vector<std::string> files;

        if (username == "user" && password == "password") {
            valid = true;
            std::string userDirectory = "./storage/16165335557417779349";
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

void WebSocketSession::doClose()
{
    beast::error_code ec;
    if (!ec) {
        m_socket.close(tcp::socket::shutdown_send, ec);
    }
    else {
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    }
    std::cout << "Session ended, socket closed." << std::endl;
}
