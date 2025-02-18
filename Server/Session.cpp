#include "Session.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;

Session::Session(tcp::socket&& socket) :
    m_socket(std::move(socket)),
    m_buffer(),
    m_ha1() 
{
    m_nonce = Digest::GenerateNonce();
}

void Session::Run() {
    asio::dispatch(
        m_socket.get_executor(),
        beast::bind_front_handler(&Session::doRead, shared_from_this()));
}

void Session::doRead() {

    m_parser.emplace();
    m_parser->body_limit(2 * 1024 * 1024);

    http::async_read(m_socket, m_buffer, *m_parser,
        beast::bind_front_handler(&Session::onRead, shared_from_this()));
}

void Session::onRead(beast::error_code er, std::size_t s) {

    if (er == http::error::end_of_stream) {
        std::cout << "End of the stream, closing the connection" << std::endl;
        return;
    }

    if (er) {
        std::cerr << "Read error: " << er.message() << std::endl;
        return;
    }

    try {
        auto request = m_parser->release();

        if (request.target() == "/webclient") {
            handleWebClientRequest(request);
            return;
        }

        if (boost::beast::websocket::is_upgrade(request)) {
            std::make_shared<WebSocketSession>(std::move(m_socket))->doAccept(std::move(request));
            return;
        }

        sendFile(request);
    }
    catch (const std::exception& ex) {
        std::cerr << "Request processing error: " << ex.what() << std::endl;
    }
}

void Session::handleWebClientRequest(http::request<http::string_body>& request) {
    if (request.find(http::field::authorization) == request.end()) {
        sendUnauthorized(request);
        return;
    }

    std::string authHeader = request[http::field::authorization];
    std::string method = request.method_string();

    if (!Digest::CheckDigest(authHeader, method)) {
        sendUnauthorized(request);
        return;
    }

    std::string username = findUsername(authHeader);

    std::ifstream htmlFile("D:\\prog\\Client-server\\WebClient\\index.html");
    std::stringstream buffer;
    buffer << htmlFile.rdbuf();
    std::string htmlContent = buffer.str();
    htmlFile.close();

    std::string placeholder = "{{USERNAME}}";
    size_t pos = htmlContent.find(placeholder);
    if (pos != std::string::npos) {
        htmlContent.replace(pos, placeholder.length(), username);
    }

    m_responce = std::make_shared<http::response<http::string_body>>(http::status::ok, request.version());
    m_responce->set(http::field::content_type, "text/html");
    m_responce->body() = htmlContent;
    m_responce->prepare_payload();

    http::async_write(m_socket, *m_responce,
        beast::bind_front_handler(&Session::onWrite, shared_from_this()));
}

void Session::sendUnauthorized(http::request<http::string_body>& request) {
    std::ostringstream authChallenge;
    authChallenge << "Digest realm=\"/audioserver\", nonce=\"" << m_nonce
        << "\", algorithm=MD5, qop=\"auth\"";

    m_responce = std::make_shared<http::response<http::string_body>>(http::status::unauthorized, request.version());
    m_responce->set(http::field::www_authenticate, authChallenge.str());
    m_responce->set(http::field::content_type, "text/plain");
    m_responce->body() = "Unauthorized";
    m_responce->prepare_payload();

    http::async_write(m_socket, *m_responce,
        beast::bind_front_handler(&Session::onWrite, shared_from_this()));
}

std::string Session::findUsername(const std::string& authHeader) {
    std::string authParamsStr = authHeader.substr(7);
    std::map<std::string, std::string> authParams;
    std::istringstream paramStream(authParamsStr);
    std::string param;

    while (std::getline(paramStream, param, ',')) {
        boost::trim(param);
        size_t pos = param.find('=');
        if (pos != std::string::npos) {
            std::string key = param.substr(0, pos);
            std::string value = param.substr(pos + 1);
            boost::trim(key);
            boost::trim(value);
            if (!value.empty() && value.front() == '"') {
                value.erase(0, 1);
            }
            if (!value.empty() && value.back() == '"') {
                value.pop_back();
            }
            authParams[key] = value;
        }
    }

    return authParams["username"];
}

void Session::sendFile(http::request<http::string_body>& request) {
    std::string token;
    if (request.find("token") != request.end()) {
        token = request["token"];
    }
    if (token.empty()) {
        std::string ha1 = request["ha1"];
        std::string username = request["username"];
        std::string authHeader = request[http::field::authorization];
        std::string method = request.method_string();

        if (ha1.empty() || username.empty() || !Digest::CheckDigest(authHeader, method, ha1)) {
            sendUnauthorized(request);
            return;
        }
        else {
            auto now = std::chrono::system_clock::now();
            auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
            std::ostringstream tokenStream;
            tokenStream << epoch;
            std::string newToken = tokenStream.str();

            std::ofstream ofs("clients.txt", std::ios::app);
            if (ofs.is_open()) {
                ofs << newToken << " " << ha1 << " " << username << std::endl;
                ofs.close();
            }
            else {
                std::cerr << "Error opening clients.txt." << std::endl;
            }

            m_responce = std::make_shared<http::response<http::string_body>>(http::status::ok, request.version());
            m_responce->set(http::field::content_type, "text/plain");
            m_responce->body() = newToken;
            m_responce->prepare_payload();
            http::async_write(m_socket, *m_responce,
                beast::bind_front_handler(&Session::onWrite, shared_from_this()));
            return;
        }
    }
    if (!validateToken(token)) {
        sendUnauthorized(request);
        return;
    }

    std::ofstream log("log.txt", std::ios::app);
    std::size_t fileSize = request.body().size();
    std::size_t directorySize = ServerStorage::GetStorageSize("./storage/" + token);

    log << "[INFO] Incoming request with file size: " << fileSize << " bytes.\n";

    if (directorySize + fileSize > MAX_SIZE) {
        log << "[INFO] Storage size exceeded. Deleting old files.\n";
        ServerStorage::DeleteFiles("./storage/" + token, MAX_SIZE, "./storage/" + token + "/file_dates.txt");
    }

    std::string filename = ServerStorage::GenerateFilename(token);

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        throw std::runtime_error("Failed to open file for writing.");
    }

    outFile.write(request.body().data(), request.body().size());
    outFile.close();

    log << "[SAVE] " << filename << " saved to storage.\n";

    ServerStorage::AddFile(std::filesystem::path(filename).filename().string(),
        "./storage/" + token + "/file_dates.txt", token);

    m_responce = std::make_shared<http::response<http::string_body>>(http::status::ok, request.version());
    m_responce->set(http::field::server, "AudioServer");
    m_responce->set(http::field::content_type, "text/plain");
    m_responce->body() = "Part of audio saved as " + filename;
    m_responce->prepare_payload();

    http::async_write(m_socket, *m_responce,
        beast::bind_front_handler(&Session::onWrite, shared_from_this()));
}

bool Session::validateToken(const std::string& token) {
    std::ifstream tokenFile("clients.txt");
    if (tokenFile.is_open()) {
        std::string storedToken, storedHa1, storedUsername;
        while (tokenFile >> storedToken >> storedHa1 >> storedUsername) {
            if (storedToken == token) {
                return true;
            }
        }
        tokenFile.close();
    }
    return false;
}

void Session::onWrite(beast::error_code ec, size_t s) {
    if (!ec) {
        m_socket.shutdown(tcp::socket::shutdown_send, ec);
    }
    else {
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    }
}
