#include "MessageHandler.hpp"

MessageHandler::MessageHandler(std::shared_ptr<asio::io_context> ioc)
    :
    m_ioc(ioc),
    m_endpoint(tcp::v4(), 8080),
    m_acceptor(*m_ioc, m_endpoint),
    request(std::make_shared<http::request<http::string_body>>())
    {
    m_acceptor.set_option(asio::socket_base::reuse_address(true));
    m_acceptor.listen();   
    }

void MessageHandler::Start()       
{
   acceptConnections();
}

void MessageHandler::acceptConnections() {
    
    auto self = shared_from_this();
    std::cout << "Waiting for request..." << std::endl;

    m_acceptor.async_accept(asio::make_strand(*m_ioc),
        [self](beast::error_code er, tcp::socket socket) {
            if (!er) {
                auto session = std::make_shared<Session>(std::move(socket));
                session->Run();

                self->acceptConnections();
            }
            else {
                std::cerr << "Accept error: " << er.message() << std::endl;
            }          
        });

}

void MessageHandler::handle(beast::error_code er, tcp::socket socket)
{
    if (!er)
    {
        auto session = std::make_shared<Session>(std::move(socket));
        session->Run();
    }
    else
    {
        std::cerr << "Accept error: " << er.message() << std::endl;
    }
    acceptConnections();
}
