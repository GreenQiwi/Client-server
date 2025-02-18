#pragma once

#include <random>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Authentication {
public:
    Authentication();

    std::string GetAuthHeader(); 

    void SetUsername(std::string username);
    void SetPassword(std::string password);

    std::string GetUsername();

    std::string GetToken();
    void SetToken(const std::string& token);

    void LogIn();  
    void Authenticate(http::response<http::string_body> res);
    std::string GenerateHa1(std::string realm);

private:   
    void md5(const std::vector<uint8_t>& input, uint8_t digest[16]);
    uint32_t left_rotate(uint32_t x, uint32_t c);
    std::string calculateMD5(const std::string& input);
    std::string generateDigest(const std::string& method, const std::string& uri,
        const std::string& nonce, const std::string& realm,
        const std::string& cnonce, const std::string& nc, const std::string& qop);
    std::string generateNonce();
    
private:
    std::string m_authHeader;

    std::string m_username;
    std::string m_password;

    std::string m_token;
};
