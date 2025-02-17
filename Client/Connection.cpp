#include "Connection.hpp"

Connection::Connection(const std::string& host, const std::string& port)
    : m_host(host), m_port(port), m_resolver(m_ioc) {}

http::response<http::string_body> Connection::UploadFile(const std::string& filename, const std::string& target, 
    const std::string& contentType, std::string& authToken, const std::string& ha1, const std::string& authHeader, const std::string& username)
{
    try
    {
        tcp::resolver resolver(m_ioc);
        auto const results = resolver.resolve(m_host, m_port);
        tcp::socket socket(m_ioc);
        beast::flat_buffer buffer;

        asio::connect(socket, results.begin(), results.end());

        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> dataVect(fileSize);
        if (!file.read(dataVect.data(), fileSize)) {
            std::cerr << "Error reading file content." << std::endl;
        }
        const std::string data(dataVect.begin(), dataVect.end());
        
        if (!authToken.empty() && authToken[authToken.size() - 1] == '\n') {
            authToken.erase(authToken.size() - 1); 
        }

        http::request<http::string_body> req{ http::verb::post, target, 11 };
        req.set(http::field::host, m_host);
        req.set(http::field::content_type, contentType);
        req.set(http::field::content_length, std::to_string(fileSize));
        req.set("token", authToken);
        req.set("ha1", ha1);
        req.set(http::field::authorization, authHeader);
        req.set("username", username);
        req.body() = data;
        req.prepare_payload();


        std::cout << "Sending request..." << std::endl;
        std::cout << "Request body size: " << req.body().size() << std::endl;
        std::cout << "Request body (first 100 chars): " << req.body().substr(0, 100) << std::endl;

        http::write(socket, req);
     
        if (!socket.is_open()) {
            std::cerr << "Failed to connect to the server." << std::endl;
        }

        http::response<http::string_body> resp;

        http::read(socket, buffer, resp);

        std::cout << "Server response: " << resp << std::endl;
        return resp;
    }
    catch (const std::exception& ex) {
        http::response<http::string_body> resp;
        return resp;
    }
}

