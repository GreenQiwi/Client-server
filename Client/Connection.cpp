#include "Connection.hpp"

Connection::Connection(const std::string& host, const std::string& port)
    : m_host(host), m_port(port) {}

void Connection::UploadFile(const std::string& filename, const std::string& target, const std::string& contentType, const std::string& login, const std::string& password)
{
    try
    {        
        tcp::resolver resolver(m_ioc);
        auto const results = resolver.resolve(m_host, m_port);
        tcp::socket socket(m_ioc);
        beast::flat_buffer buffer;

        asio::connect(socket, results.begin(), results.end());

        //if (!socket.is_open()) {
        //    std::cerr << "Failed to connect to the server." << std::endl;
        //    return;
        //}
        //
        //http::request<http::empty_body> initialReq{ http::verb::post, target, 11 };
        //initialReq.set(http::field::host, m_host);
        //initialReq.set(http::field::content_length, "0");
        //initialReq.prepare_payload();
        //
        //std::cout << "Sending initial request..." << std::endl;
        //std::cout << "Request method: " << initialReq.method() << std::endl;
        //
        //try {
        //    http::write(socket, initialReq);
        //}
        //catch (const boost::system::system_error& ex) {
        //    std::cerr << "Boost error: " << ex.what() << std::endl;
        //    return;
        //}
        //
        //
        //
        //http::response<http::string_body> initialResp;
        //try {
        //    http::read(socket, buffer, initialResp);
        //}
        //catch (const boost::system::system_error& ex) {
        //    std::cerr << "Boost error: " << ex.what() << std::endl;
        //    return;
        //}
        //
        //if (initialResp.result() != http::status::unauthorized) {
        //    std::cerr << "Expected 401 Unauthorized, but received: " << initialResp.result() << std::endl;
        //    return;
        //}
        //
        //std::string authHeader = std::string(initialResp[http::field::www_authenticate]);
        //if (authHeader.empty() || authHeader.find("Digest") == std::string::npos) {
        //    std::cerr << "Invalid WWW-Authenticate header." << std::endl;
        //    return;
        //}
        //
        //std::string realm, nonce;
        //size_t realmPos = authHeader.find("realm=\"");
        //size_t noncePos = authHeader.find("nonce=\"");
        //if (realmPos != std::string::npos) {
        //    realmPos += 7; 
        //    size_t end = authHeader.find("\"", realmPos);
        //    realm = authHeader.substr(realmPos, end - realmPos);
        //}
        //if (noncePos != std::string::npos) {
        //    noncePos += 7; 
        //    size_t end = authHeader.find("\"", noncePos);
        //    nonce = authHeader.substr(noncePos, end - noncePos);
        //}
        //
        //if (realm.empty() || nonce.empty()) {
        //    std::cerr << "Failed to extract realm or nonce." << std::endl;
        //    return;
        //}
        
        std::string method = "POST";
        std::string uri = target;
        std::string digestResponse = Authentication::GenerateDigest(method, uri, login, password, "nonce", "realm");

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
        req.set(http::field::host, m_host);
        req.set(http::field::content_type, contentType);
        req.set(http::field::content_length, std::to_string(fileSize));
        req.set("Authorization", "Digest username=\"" + login +
            "\", realm=\"" + "realm" +
            "\", nonce=\"" + "nonce" +
            "\", uri=\"" + uri +
            "\", response=\"" + digestResponse + "\"");
        req.body() = data;
        req.prepare_payload();
        if (!socket.is_open()) {
            std::cerr << "Failed to connect to the server." << std::endl;
            return;
        }

        std::cout << "Sending request..." << std::endl;
        std::cout << "Request body size: " << req.body().size() << std::endl;
        std::cout << "Request body (first 100 chars): " << req.body().substr(0, 100) << std::endl;

        http::write(socket, req);
        if (!socket.is_open()) {
            std::cerr << "Failed to connect to the server." << std::endl;
            return;
        }

        http::response<http::string_body> resp;
        
        try {
            http::read(socket, buffer, resp);
        }
        catch (const boost::system::system_error& ex) {
            std::cerr << "Boost error: " << ex.what() << std::endl;
            return;
        }

        std::cout << "Server response: " << resp << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "File upload failed: " << ex.what() << std::endl;
    }
}

