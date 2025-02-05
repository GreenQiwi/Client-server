#include "Connection.hpp"

Connection::Connection(const std::string& host, const std::string& port)
    : m_host(host), m_port(port), m_resolver(m_ioc) {}


//std::string Connection::Authenticate() {
//    try {   
//        auto const results = m_resolver.resolve(m_host, m_port);
//        auto s = asio::connect(m_socket, results.begin(), results.end());
//        m_socket.set_option(asio::ip::tcp::no_delay(true));
//        std::ifstream clientFile("client_id.txt");
//        if (clientFile.is_open()) {
//            clientFile >> m_clientId;
//        }
//        clientFile.close();
//
//        std::string ha1 = m_auth.GenerateHa1("/audioserver");
//
//        beast::flat_buffer buffer;
//        http::request<http::string_body> req{ http::verb::post, "AUTH", 11 }; // 11 const
//        req.set(http::field::host, m_host);
//        req.body() = ha1 + " " + m_clientId;
//        //req.prepare_payload();
//        http::write(m_socket, req);
//
//        http::response<http::string_body> res;
//        //http::read(m_socket, buffer, res);
//
//        //std::string responseBody = res.body();
//        //
//        //if (responseBody.find("New client-id generated: ") == 0) {
//        //    m_clientId = responseBody.substr(24);
//        //    std::ofstream clientFileOut("client_id.txt");
//        //    clientFileOut << m_clientId;
//        //    clientFileOut.close();
//        //    std::cout << "New client-id received and saved: " << m_clientId << std::endl;
//        //}
//        //else {
//        //    std::cout << "Authentication successful: " << responseBody << std::endl;
//        //}
//        //
//        //auto authHeader = res[http::field::www_authenticate];
//        //std::string realm, nonce;
//        //
//        //auto realmPos = authHeader.find("realm=\"");
//        //if (realmPos != std::string::npos) {
//        //    realmPos += 7;
//        //    auto end = authHeader.find("\"", realmPos);
//        //    realm = authHeader.substr(realmPos, end - realmPos);
//        //}
//        //
//        //auto noncePos = authHeader.find("nonce=\"");
//        //if (noncePos != std::string::npos) {
//        //    noncePos += 11;
//        //    auto end = authHeader.find("\"", noncePos);
//        //    nonce = authHeader.substr(noncePos, end - noncePos);
//        //}
//        //
//        //if (realm.empty() || nonce.empty()) {
//        //    throw std::runtime_error("Failed to extract realm or nonce from WWW-Authenticate header.");
//        //}        
//        //
//        //m_authHeader = "Digest username=\"" + m_auth.GetUsername() +
//        //    "\", realm=\"" + realm +
//        //    "\", nonce=\"" + nonce +
//        //    "\", uri=\"/audioserver\", algorithm=MD5, qop=\"auth\", response=\"" +
//        //    m_auth.GenerateDigest("POST", "/audioserver", nonce, realm) + "\"";
//        //
//        //std::cout << "Authentication header generated: " << m_authHeader << std::endl;
//        //
//        //return m_authHeader;
//        return "";
//    }
//    catch (const std::exception& ex) {
//        std::cerr << "Authentication failed: " << ex.what() << std::endl;
//    }
//}


http::response<http::string_body> Connection::UploadFile(const std::string& filename, const std::string& target, const std::string& contentType, std::string& authToken, const std::string& digestHeader)
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
        req.set(http::field::authorization, authToken);
        req.set("ha1", digestHeader);
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
        std::cerr << "File upload failed: " << ex.what() << std::endl;
    }
}

