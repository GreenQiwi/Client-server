#include "Connection.hpp"

Connection::Connection(const std::string& host, const std::string& port)
    : host(host), port(port) {}

void Connection::UploadFile(const std::string& filename, const std::string& target, const std::string& contentType, const std::string& login, const std::string& password) 
{
    try 
    {
        asio::io_context ioc;
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);
        tcp::socket socket(ioc);
        asio::connect(socket, results.begin(), results.end());

        std::string nonce = "nonce"; 
        std::string realm = "realm"; 
        std::string method = "POST";
        std::string uri = target;

        std::string digestResponse = Authentication::generateDigest(method, uri, login, password, nonce, realm);


        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> dataVect(fileSize);
        if (!file.read(dataVect.data(), fileSize)) {
            std::cerr << "Error reading file content." << std::endl;
            return;
        }
        const std::string data(dataVect.begin(), dataVect.end());

        http::request<http::string_body> req{ http::verb::post, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::content_type, contentType);
        req.set(http::field::content_length, std::to_string(fileSize));

        req.set("Authorization", "Digest username=\"" + login + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri + "\", response=\"" + digestResponse + "\"");
        req.body() = data;
        req.prepare_payload();


        http::write(socket, req);
        beast::flat_buffer buffer;
        http::response<http::string_body> resp;
        http::read(socket, buffer, resp);

        std::cout << "Server response: " << resp << std::endl;


    }
    catch (const std::exception& ex) {
        std::cerr << "File upload failed: " << ex.what() << std::endl;
    }
}
