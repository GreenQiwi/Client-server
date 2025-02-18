#include "MessageHandler.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

int main() {
    setlocale(LC_ALL, "rus");

    try {
        int hardwareConcurrency = std::thread::hardware_concurrency();
        int numOfThreads = hardwareConcurrency > 0 ? hardwareConcurrency : 1;
        auto ioc = std::make_shared<asio::io_context>(numOfThreads);
        auto handler = std::make_shared<MessageHandler>(ioc);
        handler->Start();
;
        std::vector<std::thread> threads;
        threads.reserve(numOfThreads);
        for (std::size_t i = 0; i < numOfThreads; ++i) {
            threads.emplace_back([ioc]() {
                ioc->run();
                });
        }
        ioc->run();
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

    }
    catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }

    return 0;
}

