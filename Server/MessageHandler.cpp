#include "MessageHandler.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;

MessageHandler::MessageHandler(tcp::acceptor acceptor)
    : acceptor(std::move(acceptor)), threadpool(std::thread::hardware_concurrency()) {}

void MessageHandler::start()
{
    acceptConnections();
}

void MessageHandler::acceptConnections()
{
    acceptor.async_accept([this](beast::error_code er, tcp::socket socket) {
        if (!er) {
            asio::post(threadpool, [this, socket = std::move(socket)]() mutable {
                readRequest(std::move(socket));
                });
        }
        else {
            std::cerr << "Accept error: " << er.message() << std::endl;
        }

        acceptConnections();
        });
}

void MessageHandler::readRequest(tcp::socket socket)
{
    auto buffer = std::make_shared<beast::flat_buffer>();
    auto parser = std::make_shared<http::request_parser<http::string_body>>();
    auto shared_socket = std::make_shared<tcp::socket>(std::move(socket));

    parser->body_limit(2 * 1024 * 1024);

    http::async_read(*shared_socket, *buffer, *parser,
        [this, shared_socket, buffer, parser](beast::error_code er, std::size_t) mutable {
            if (!er) {
                try {
                    const auto& req = parser->get();

                    std::ofstream log("log.txt", std::ios::app);
                    std::string login = req["login"];
                    std::string password = req["password"];
                    std::size_t fileSize = req.body().size();

                    std::size_t directorySize = ServerStorage::getStorageSize("./storage/" + login);

                    log << "[INFO] Incoming request with file size: " << fileSize << " bytes.\n";                    

                    if (directorySize + fileSize > MAX_SIZE) {
                        log << "[INFO] Storage size exceeded. Deleting old files.\n";
                        ServerStorage::deleteFiles("./storage/" + login, MAX_SIZE, "./storage/" + login + "/file_dates.txt");
                    }
                  

                    std::string filename = ServerStorage::generateFilename(login);

                    std::ofstream outFile(filename, std::ios::binary);
                    if (!outFile.is_open()) {
                        std::cerr << "Failed to open file for writing: " << filename << std::endl;
                        throw std::runtime_error("Failed to open file for writing.");
                    }

                    outFile.write(req.body().data(), req.body().size());
                    outFile.close();

                    log << "[SAVE] " << filename << " saved to storage.\n";

                    ServerStorage::addFile(std::filesystem::path(filename).filename().string(), "./storage/" + login + "/file_dates.txt", login);

                    http::response<http::string_body> res{ http::status::ok, req.version() };
                    res.set(http::field::server, "AudioServer");
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Part of audio saved as " + filename;
                    res.prepare_payload();
                    http::async_write(*shared_socket, res, [shared_socket](beast::error_code er, std::size_t) {
                        if (er) {
                            std::cerr << "Write error: " << er.message() << std::endl;
                        }
                        shared_socket->shutdown(tcp::socket::shutdown_send); 
                        });
                }
                catch (const std::exception& ex) {
                    std::cerr << "Request processing error: " << ex.what() << std::endl;
                }
            }
            else {
                std::cerr << "Read error: " << er.message() << std::endl;
            }
        });
}

/*void MessageHandler::readRequest(tcp::socket socket)
{
    beast::flat_buffer buffer;
    std::cout << buffer.data().data();
    http::request_parser<http::string_body> parser;
    parser.body_limit(2 * 1024 * 1024);

    http::async_read(socket, buffer, parser,
        [this, socket = std::move(socket), &buffer, &parser](beast::error_code er, std::size_t) mutable {
            if (!er) {
                try {
                    const auto& req = parser.get();
                    std::ofstream log("log.txt", std::ios::app);
                    std::size_t fileSize = req.body().size();
                    std::size_t directorySize = ServerStorage::getStorageSize("./storage");

                    log << "[INFO] Incoming request with file size: " << fileSize << " bytes.\n";
                    log << "[INFO] Buffer content: " << buffer.size() << " bytes.\n";

                    if (directorySize + fileSize > MAX_SIZE) {
                        log << "[INFO] Storage size exceeded. Deleting old files.\n";
                        ServerStorage::deleteFiles("./storage", MAX_SIZE, DATE_FILE);
                    }

                    std::string filename = ServerStorage::generateFilename();

                    std::ofstream outFile(filename, std::ios::binary);
                    if (!outFile.is_open()) {
                        throw std::runtime_error("Failed to open file for writing.");
                    }

                    outFile.write(req.body().data(), req.body().size());
                    outFile.close();

                    log << "[SAVE] " << filename << " saved to storage.\n";
                    ServerStorage::addFile(std::filesystem::path(filename).filename().string(), DATE_FILE);

                    http::response<http::string_body> res{ http::status::ok, req.version() };
                    res.set(http::field::server, "AudioServer");
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Part of audio saved as " + filename;
                    res.prepare_payload();

                    log << "[INFO] Sending response with status: " << res.result_int() << ".\n";

                    http::async_write(socket, res, [](beast::error_code er, std::size_t) {
                        if (er) {
                            std::cerr << "Write error: " << er.message() << std::endl;
                        }
                        });
                }
                catch (const std::exception& ex) {
                    std::cerr << "Request processing error: " << ex.what() << std::endl;
                }
            }
            else {
                std::cerr << "Read error: " << er.message() << std::endl;
            }
        });
}*/






