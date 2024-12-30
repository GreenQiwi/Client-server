#include "MessageHandler.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;


int main()
{
    setlocale(LC_ALL, "rus");
    try
    {
        asio::io_context ioc;
        tcp::endpoint endpoint(tcp::v4(), 8080);
        tcp::acceptor acceptor(ioc, endpoint);

        MessageHandler handler(std::move(acceptor));
        handler.Start();
        ioc.run();

    }
    catch (const std::exception& ex)
    {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }
    return 0;
}
