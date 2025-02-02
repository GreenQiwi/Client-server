#include "Session.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;

Session::Session(tcp::socket&& socket) :
    m_socket(std::move(socket)),
    m_buffer(),
    m_ha1() {
    m_nonce = Digest::GenerateNonce();
    std::cout << "Session created" << std::endl;
    };


Session::~Session() {
    std::cout << "Session destroyed" << std::endl;
}
void Session::Run()
{
    asio::post(m_socket.get_executor(),
        beast::bind_front_handler(&Session::doRead, shared_from_this()));
}

void Session::doRead()
{
    std::cout << "doRead() called" << std::endl;

    m_parser.emplace();
    m_parser->body_limit(2 * 1024 * 1024);
    
    http::async_read(m_socket, m_buffer, *m_parser,
        beast::bind_front_handler(&Session::onRead, shared_from_this()));
}

void Session::onRead(beast::error_code er, std::size_t s)
{
    std::cout << "onRead() called" << std::endl;

    if (er == http::error::end_of_stream) {
        std::cout << "End of the stream, closing the connection" << std::endl;
        doClose();
        return;
    }

    if (er) {
        std::cerr << "Read error: " << er.message() << std::endl;
        return;
    }

    try {
        auto request = m_parser->release();

        std::cout << "Request body size: " << request.body().size() << " bytes." << std::endl;
        std::cout << "Request body (first 100 chars): " << request.body().substr(0, 100) << std::endl;

        std::string token = request[http::field::authorization];
        bool tokenValid = false;

        if (!token.empty()) {

            std::ifstream tokenFile("clients.txt");
            if (tokenFile.is_open()) {
                std::string storedToken;
                while (std::getline(tokenFile, storedToken)) {
                    if (storedToken == token) {
                        tokenValid = true;
                        break;
                    }
                }
                tokenFile.close();
            }
        }

        if (!tokenValid) {
            if (!Digest::CheckDigest(request, m_ha1, m_nonce)) {
                auto now = std::chrono::system_clock::now();
                auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                std::ostringstream clientIdStream;
                clientIdStream << epoch;
                std::string clientId = clientIdStream.str();

                std::ostringstream authHeader;
                authHeader << "Digest realm=\"" << clientId
                    << "\", nonce=\"" << m_nonce
                    << "\", algorithm=MD5, qop=\"auth\"";

                http::response<http::string_body> response(http::status::unauthorized, request.version());
                response.set(http::field::www_authenticate, authHeader.str());
                response.set(http::field::content_type, "text/plain");
                response.body() = "Unauthorized";
                response.prepare_payload();

                http::async_write(m_socket, response,
                    beast::bind_front_handler(&Session::onWrite, shared_from_this()));
                doClose();
                return;
            }
            else {
                auto now = std::chrono::system_clock::now();
                auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                std::ostringstream tokenStream;
                tokenStream << "token_" << epoch;
                std::string newToken = tokenStream.str();

                std::ofstream ofs("clients.txt", std::ios::app);
                if (!ofs.is_open()) {
                    std::cerr << "Ќе удалось открыть clients.txt дл€ записи нового токена." << std::endl;
                }
                else {
                    ofs << newToken << std::endl;
                    ofs.close();
                }

                http::response<http::string_body> response(http::status::ok, request.version());
                response.set(http::field::content_type, "text/plain");
                response.body() = newToken;
                response.prepare_payload();

                http::async_write(m_socket, response,
                    beast::bind_front_handler(&Session::onWrite, shared_from_this()));
                doClose();
                return;
            }
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

        http::response<http::string_body> response(http::status::ok, request.version());
        response.set(http::field::server, "AudioServer");
        response.set(http::field::content_type, "text/plain");
        response.body() = "Part of audio saved as " + filename;
        response.prepare_payload();

        http::async_write(m_socket, response,
            beast::bind_front_handler(&Session::onWrite, shared_from_this()));
        doClose();
        return;
    }
    catch (const std::exception& ex) {
        std::cerr << "Request processing error: " << ex.what() << std::endl;
    }
}


void Session::doClose()
{
    beast::error_code ec;
    if (!ec) {
        m_socket.shutdown(tcp::socket::shutdown_send, ec);
    }
    else {
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    }
    std::cout << "Session ended, socket closed." << std::endl;
}

void Session::onWrite(beast::error_code er, std::size_t)
{
    std::cout << "onWrite() called" << std::endl;

    if (er) {
        std::cerr << "Write error: " << er.message() << std::endl;
    }

    doRead();
}
