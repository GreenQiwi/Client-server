#include <iostream>
#include <fstream>
#include <filesystem>
#include <boost/config.hpp>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

const int MAX_SIZE = 10 * 1024 * 1024;

std::size_t getStorageSize(const std::string& directory)
{
    std::size_t size = 0;
    for (const auto& file : std::filesystem::directory_iterator(directory))
    {
        size += std::filesystem::file_size(file);
    }
    return size;

}

std::string generateFilename()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);

    std::tm time_info;
    localtime_s(&time_info, &time_now);

    std::ostringstream oss;
    oss << "./storage/audio_part_"
        << std::put_time(&time_info, "%Y%m%d%H%M%S")
        << ".wav";

    return oss.str();
}

void deleteFiles(const std::string& directory, std::size_t maxSize, std::ofstream& log)
{

    std::vector<std::filesystem::path> files;
    size_t size = 0;

    for (const auto& file : std::filesystem::directory_iterator(directory))
    {
        if (file.is_regular_file() && file.path().filename().string().find("audio_part_") == 0)
        {
            files.push_back(file.path());
            size += std::filesystem::file_size(file.path());
        }
    }

    if (size <= maxSize)
    {
        return;
    }

    std::sort(std::begin(files), std::end(files), [](std::filesystem::path a, std::filesystem::path b)
        {
            return a.filename().string() < b.filename().string();
        }
    );

    for (auto file : files)
    {
        size -= std::filesystem::file_size(file);
        std::filesystem::remove(file);
        log << file.filename() << " is deleted\n";

        if (size <= maxSize)
        {
            break;
        }
    }
}

void handleRequest(beast::tcp_stream& stream, beast::flat_buffer& buffer)
{
    try {
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
        std::size_t directorySize = getStorageSize("./storage");

        if (directorySize + fileSize > MAX_SIZE)
        {
            deleteFiles("./storage", MAX_SIZE, log);
        }


        std::string filename = generateFilename();

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


int main()
{
    try {

        asio::io_context ioc;
        tcp::acceptor acc(ioc, tcp::endpoint(tcp::v4(), 8080));

        while (true)
        {
            tcp::socket socket(ioc);
            acc.accept(socket);
            beast::tcp_stream stream(std::move(socket));
            beast::flat_buffer buffer;

            handleRequest(stream, buffer);
            stream.socket().shutdown(tcp::socket::shutdown_send);
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }
    return 0;
}