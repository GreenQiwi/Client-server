#include "MessageHandler.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;


int main()
{
    try 
    {

        asio::io_context ioc;
        tcp::acceptor acc(ioc, tcp::endpoint(tcp::v4(), 8080));

        while (true)
        {
            tcp::socket socket(ioc);
            acc.accept(socket);
            beast::tcp_stream stream(std::move(socket));
            beast::flat_buffer buffer;

            MessageHandler::handleRequest(stream, buffer);
            stream.socket().shutdown(tcp::socket::shutdown_send);
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }
    return 0;
}