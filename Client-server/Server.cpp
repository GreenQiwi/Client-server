#include <iostream>
#include <fstream>
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

int fileCounter = 0;

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
        std::cout << "Request headers:" << std::endl;

        std::string filename = "audio_part_" + std::to_string(fileCounter++) + ".wav";
        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile.is_open())
        {
            std::cerr << "Error opening file" << std::endl;
            return;
        }
        outFile.write(req.body().data(), req.body().size());
        outFile.close();

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
        tcp::acceptor acc(ioc ,tcp::endpoint(tcp::v4(), 8080));

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