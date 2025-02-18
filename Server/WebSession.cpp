#include "WebSession.hpp"

WebSocketSession::WebSocketSession(tcp::socket&& socket)
    : m_socket(std::move(socket)) {}

void WebSocketSession::doAccept(http::request<http::string_body> req) {
    m_socket.async_accept(req,
        beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this()));
}

void WebSocketSession::onAccept(beast::error_code ec) {
    if (ec) {
        std::cerr << "WebSocket accept error: " << ec.message() << std::endl;
        return;
    }
    doRead();
}

void WebSocketSession::doRead() {
    m_socket.async_read(m_buffer,
        beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t bytesTransferred) {
    boost::ignore_unused(bytesTransferred);
    if (ec == websocket::error::closed) { 
        return;
    }
    if (ec) {
        std::cerr << "WebSocket read error: " << ec.message() << std::endl;
        return;
    }

    std::string message = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(m_buffer.size());

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
    else if (command == "deleteFile") {
        std::string username, fileName;
        std::getline(iss, username, ':');
        std::getline(iss, fileName);

        if (getUserDirectory(username)) {
            deleteFile(fileName);
        }
        else {
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
    if (fileData.empty()) {
        sendMessage("{\"error\":\"File is empty\"}");
        return;
    }

    std::string fileDataBase64 = encodeBase64(fileData);

    m_message = "{\"type\":\"fileData\",\"fileName\":\"" + filename + "\",\"data\":\"" + fileDataBase64 + "\"}";

    sendMessage(m_message);
}

std::string WebSocketSession::encodeBase64(const std::vector<char>& input) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::vector<char>::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(input)), It(std::end(input)));
    return tmp.append((3 - input.size() % 3) % 3, '=');
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
}

std::map<std::string, std::time_t> WebSocketSession::ReadAssociations(const std::string& file) {
    std::map<std::string, std::time_t> associations;
    std::ifstream inFile(file);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open associations file: " << file << std::endl;
        return associations;
    }

    std::string filename;
    std::time_t timestamp;
    while (inFile >> filename >> timestamp) {
        associations[filename] = timestamp;
    }

    inFile.close();
    return associations;
}

void WebSocketSession::WriteAssociations(const std::map<std::string, std::time_t>& associations, const std::string& file) {
    std::ofstream outFile(file);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open associations file: " << file << std::endl;
        return;
    }

    for (const auto& [filename, timestamp] : associations) {
        outFile << filename << " " << timestamp << "\n";
    }

    outFile.close();
}

void WebSocketSession::deleteFile(const std::string& fileName) {
    if (m_userDirectory.empty()) {
        std::cerr << "User directory is not set!" << std::endl;
        sendMessage("{\"error\":\"User directory not found\"}");
        return;
    }

    std::string filePath = m_userDirectory + "/" + fileName;
    std::string associationsFile = m_userDirectory + "/file_dates.txt";

    if (!std::filesystem::exists(filePath)) {
        sendMessage("{\"error\":\"File not found\"}");
        return;
    }

    std::filesystem::remove(filePath);
    auto associations = ReadAssociations(associationsFile);

    std::ofstream log("log.txt", std::ios::app);
    if (!log.is_open()) {
        throw std::runtime_error("Failed to open log file.");
    }
    log << "[DELETE] " << filePath <<  "deleted because of user request.\n";

    if (associations.erase(fileName) > 0) {
        WriteAssociations(associations, associationsFile);
        sendMessage("{\"type\":\"fileDeleted\",\"fileName\":\"" + fileName + "\"}");
    }
    else {
        sendMessage("{\"type\":\"fileDeleted\",\"fileName\":\"" + fileName + "\",\"warning\":\"File not found in associations\"}");
    }
}

