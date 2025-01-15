#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include "Connection.hpp"

class Authentication {
public:
    Authentication(const std::string& host, const std::string& port);

    void Authenticate(const std::string& method, const std::string& target, const std::string& username, const std::string& password);
    std::string GetAuthHeader() const { return m_authHeader; }

private:   
    void md5(const std::vector<uint8_t>& input, uint8_t digest[16]);
    uint32_t left_rotate(uint32_t x, uint32_t c);
    std::string calculateMD5(const std::string& input);

    std::string GenerateDigest(const std::string& method, const std::string& uri,
        const std::string& username, const std::string& password,
        const std::string& nonce, const std::string& realm);

    
private:
    std::string m_host;
    std::string m_port;
    std::string m_authHeader;

};
