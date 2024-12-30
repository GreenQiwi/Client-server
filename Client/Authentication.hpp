#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include "Connection.hpp"

class Authentication {
public:
    Authentication();
    void Authenticate();
    static std::string GenerateDigest(const std::string& method, const std::string& uri, const std::string& login, const std::string& password, const std::string& nonce, const std::string& realm);

public:
    std::string m_Login;
    std::string m_Password;
    std::string m_AuthHeader;

private:   
    static void md5(const std::vector<uint8_t>& input, uint8_t digest[16]);
    static uint32_t left_rotate(uint32_t x, uint32_t c);
    static std::string calculateMD5(const std::string& input);

};
