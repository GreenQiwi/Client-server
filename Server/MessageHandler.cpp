#include "MessageHandler.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;

void MessageHandler::handleRequest(beast::tcp_stream& stream, beast::flat_buffer& buffer)
{
    try
    {
        http::request<http::string_body> req;
        http::request_parser<http::string_body> parser{ std::move(req) };
        parser.body_limit(2 * 1024 * 1024);
        http::read(stream, buffer, parser);
        req = parser.release();

        std::cout << "Request method: " << req.method_string() << std::endl;
        std::cout << "Request target: " << req.target() << std::endl;
        std::cout << "HTTP version: " << req.version() << std::endl;
        std::ofstream log("log.txt", std::ios::app);

        std::size_t fileSize = req.body().size();
        std::size_t directorySize = ServerStorage::getStorageSize("./storage");

        if (directorySize + fileSize > MAX_SIZE)
        {
            ServerStorage::deleteFiles("./storage", MAX_SIZE, log);
        }


        std::string filename = ServerStorage::generateFilename();

        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile.is_open())
        {
            std::cerr << "Error opening file" << std::endl;
            return;
        }
        outFile.write(req.body().data(), req.body().size());
        outFile.close();

        log << filename << " is saved.\n";

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
    }
}