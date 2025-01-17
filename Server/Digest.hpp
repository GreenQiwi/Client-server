#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <random>
#include <fstream>
#include <iostream>
#include <map>

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;


namespace Digest{
	uint32_t left_rotate(uint32_t x, uint32_t c);
	void MD5(const std::vector<uint8_t>& input, uint8_t digest[16]);
	std::string GenerateNonce();
	bool CheckDigest(http::request<http::string_body>& req, const std::string& password);
	std::string calculateMD5(const std::string& input);
	std::string GenerateDigest(const std::string& method, const std::string& uri,
		const std::string& username, const std::string& password,
		const std::string& nonce, const std::string& realm);
	void AddUser(const std::string& login, const std::string& password);
	std::unordered_map<std::string, std::string> LoadUsers();
};
