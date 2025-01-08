#include "Session.hpp"


const int MAX_SIZE = 10 * 1024 * 1024;

Session::Session(tcp::socket&& socket) :
    socket(std::move(socket)),
    buffer() {};

Session::~Session() {
    std::cout << "Session destroyed" << std::endl;
}
void Session::Run()
{
    asio::post(socket.get_executor(),
        beast::bind_front_handler(&Session::doRead, shared_from_this()));
}

void Session::doRead()
{
    parser.emplace();
    parser->body_limit(2 * 1024 * 1024);
    
    http::async_read(socket, buffer, *parser,
        beast::bind_front_handler(&Session::onRead, shared_from_this()));
}

void Session::onRead(beast::error_code er, std::size_t)
{
    if (!er) {
        try {
            std::string requiredPassword = "default";
            auto request = parser->release();

            std::cout << "Request body size: " << request.body().size() << " bytes." << std::endl;
            std::cout << "Request body (first 100 chars): " << request.body().data() << std::endl;

            if (!Digest::CheckDigest(request, requiredPassword)) {

                std::string realm = "realm";
                std::string nonce = Digest::GenerateNonce();

                std::ostringstream authHeader;
                authHeader << "Digest realm=\"" << realm
                    << "\", nonce=\"" << "nonce"
                    << "\", algorithm=MD5, qop=\"auth\"";

                auto response = std::make_shared<http::response<http::string_body>>(
                    http::status::unauthorized, request.version());
                response->set(http::field::www_authenticate, authHeader.str());
                response->set(http::field::content_type, "text/plain");
                response->body() = "Unauthorized";
                response->prepare_payload();

                http::async_write(socket, *response,
                    beast::bind_front_handler(&Session::onWrite, shared_from_this()));
                return;
            }

            std::ofstream log("log.txt", std::ios::app);
            std::string login = request["login"];
            std::string password = request["password"];
            std::size_t fileSize = request.body().size();

            std::size_t directorySize = ServerStorage::GetStorageSize("./storage/" + login);

            log << "[INFO] Incoming request with file size: " << fileSize << " bytes.\n";

            if (directorySize + fileSize > MAX_SIZE) {
                log << "[INFO] Storage size exceeded. Deleting old files.\n";
                ServerStorage::DeleteFiles("./storage/" + login, MAX_SIZE, "./storage/" + login + "/file_dates.txt");
            }

            std::string filename = ServerStorage::GenerateFilename(login);

            std::ofstream outFile(filename, std::ios::binary);
            if (!outFile.is_open()) {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
                throw std::runtime_error("Failed to open file for writing.");
            }

            outFile.write(request.body().data(), request.body().size());
            outFile.close();

            log << "[SAVE] " << filename << " saved to storage.\n";


            ServerStorage::AddFile(std::filesystem::path(filename).filename().string(),
                "./storage/" + login + "/file_dates.txt", login);

            http::response<http::string_body> response{ http::status::ok, request.version() };
            response.set(http::field::server, "AudioServer");
            response.set(http::field::content_type, "text/plain");
            response.body() = "Part of audio saved as " + filename;
            response.prepare_payload();

            http::async_write(socket, response,
                beast::bind_front_handler(&Session::onWrite, shared_from_this()));
        }
        catch (const std::exception& ex) {
            std::cerr << "Request processing error: " << ex.what() << std::endl;
        }
    }
    else {
        std::cerr << "Read error: " << er.message() << " " << er.value() << std::endl;
    }

}

void Session::onWrite(beast::error_code write_er, std::size_t)
{
    if (write_er) {
        std::cerr << "Write error: " << write_er.message() << std::endl;
    }

    doRead();
}
