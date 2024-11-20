#include "MessageHandler.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;
const std::string DATE_FILE = "./storage/file_dates.txt";

void MessageHandler::handleRequest(beast::tcp_stream& stream, beast::flat_buffer& buffer)
{
    try {
        http::request<http::string_body> req;
        http::request_parser<http::string_body> parser{ std::move(req) };
        parser.body_limit(2 * 1024 * 1024); 
        http::read(stream, buffer, parser);
        req = parser.release();

        std::ofstream log("log.txt", std::ios::app);

        std::size_t fileSize = req.body().size();
        std::size_t directorySize = ServerStorage::getStorageSize("./storage");

        log << "[INFO] Incoming request with file size: " << fileSize << " bytes.\n";

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
        http::write(stream, res);
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Handling request failed. " << ex.what() << std::endl;

        std::ofstream log("log.txt", std::ios::app);
        if (log.is_open()) {
            log << "[ERROR] Handling request failed: " << ex.what() << "\n";
        }
    }
}