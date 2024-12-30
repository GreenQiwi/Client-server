#include "MessageHandler.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;



uint32_t left_rotate(uint32_t x, uint32_t c) {
    return (x << c) | (x >> (32 - c));
}

void md5(const std::vector<uint8_t>& input, uint8_t digest[16]) {
    uint32_t K[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };

    uint32_t s[64] = {
        7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
        5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
        4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
        6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
    };

    uint32_t a0 = 0x67452301;
    uint32_t b0 = 0xefcdab89;
    uint32_t c0 = 0x98badcfe;
    uint32_t d0 = 0x10325476;

    std::vector<uint8_t> padded_input = input;

    size_t original_len = padded_input.size();
    padded_input.push_back(0x80);

    while (padded_input.size() % 64 != 56) {
        padded_input.push_back(0x00);
    }

    uint64_t bit_len = original_len * 8;
    padded_input.push_back(static_cast<uint8_t>(bit_len));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 8));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 16));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 24));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 32));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 40));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 48));
    padded_input.push_back(static_cast<uint8_t>(bit_len >> 56));

    for (size_t i = 0; i < padded_input.size(); i += 64) {
        uint32_t M[16];
        for (int j = 0; j < 16; ++j) {
            M[j] = 0;
            for (int k = 0; k < 4; ++k) {
                M[j] |= static_cast<uint32_t>(padded_input[i + j * 4 + k]) << (8 * k);
            }
        }

        uint32_t A = a0, B = b0, C = c0, D = d0;

        for (int i = 0; i < 64; ++i) {
            uint32_t F, g;

            if (i < 16) {
                F = (B & C) | (~B & D);
                g = i;
            }
            else if (i < 32) {
                F = (D & B) | (~D & C);
                g = (5 * i + 1) % 16;
            }
            else if (i < 48) {
                F = B ^ C ^ D;
                g = (3 * i + 5) % 16;
            }
            else {
                F = C ^ (B | ~D);
                g = (7 * i) % 16;
            }

            F += A + K[i] + M[g];
            A = D;
            D = C;
            C = B;
            B += left_rotate(F, s[i]);
        }

        a0 += A;
        b0 += B;
        c0 += C;
        d0 += D;
    }

    digest[0] = static_cast<uint8_t>(a0);
    digest[1] = static_cast<uint8_t>(a0 >> 8);
    digest[2] = static_cast<uint8_t>(a0 >> 16);
    digest[3] = static_cast<uint8_t>(a0 >> 24);
    digest[4] = static_cast<uint8_t>(b0);
    digest[5] = static_cast<uint8_t>(b0 >> 8);
    digest[6] = static_cast<uint8_t>(b0 >> 16);
    digest[7] = static_cast<uint8_t>(b0 >> 24);
    digest[8] = static_cast<uint8_t>(c0);
    digest[9] = static_cast<uint8_t>(c0 >> 8);
    digest[10] = static_cast<uint8_t>(c0 >> 16);
    digest[11] = static_cast<uint8_t>(c0 >> 24);
    digest[12] = static_cast<uint8_t>(d0);
    digest[13] = static_cast<uint8_t>(d0 >> 8);
    digest[14] = static_cast<uint8_t>(d0 >> 16);
    digest[15] = static_cast<uint8_t>(d0 >> 24);
}




bool checkDigest(const http::request<http::string_body>& req, const std::string& password) {
    auto authHeaderIt = req.find("Authorization");
    if (authHeaderIt != req.end())
        return false;
    
    std::string authHeader = req["Authorization"];
    if (authHeader.find("Digest") == std::string::npos) {
        return false;
    }

    std::map<std::string, std::string> digestParams;
    size_t start = authHeader.find("Digest") + 7;
    std::string paramsString = authHeader.substr(start);
    std::istringstream paramStream(paramsString);
    std::string param;

    while (std::getline(paramStream, param, ',')) {
        auto eqPos = param.find('=');
        if (eqPos != std::string::npos) {
            std::string key = param.substr(0, eqPos);
            std::string value = param.substr(eqPos + 1);

            key.erase(remove_if(key.begin(), key.end(), isspace), key.end());
            value.erase(remove_if(value.begin(), value.end(), isspace), value.end()); 
            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            digestParams[key] = value;
        }
    }

    std::string username = digestParams["username"];
    std::string realm = digestParams["realm"];
    std::string nonce = digestParams["nonce"];
    std::string uri = digestParams["uri"];
    std::string response = digestParams["response"];

    if (username.empty() || realm.empty() || nonce.empty() || uri.empty() || response.empty()) {
        std::cerr << "Missing required Digest parameters." << std::endl;
        return false;
    }

    std::string ha1Input = username + ":" + realm + ":" + password;
    unsigned char ha1Digest[16];
    md5(std::vector<uint8_t>(ha1Input.begin(), ha1Input.end()), ha1Digest);

    std::ostringstream ha1HexStream;
    for (unsigned char byte : ha1Digest) {
        ha1HexStream << std::setw(2) << std::setfill('0') << std::hex << (int)byte;
    }
    std::string ha1Hex = ha1HexStream.str();

    std::string method = "POST";
    std::string ha2Input = method + ":" + uri;
    unsigned char ha2Digest[16];
    md5(std::vector<uint8_t>(ha2Input.begin(), ha2Input.end()), ha2Digest);

    std::ostringstream ha2HexStream;
    for (unsigned char byte : ha2Digest) {
        ha2HexStream << std::setw(2) << std::setfill('0') << std::hex << (int)byte;
    }
    std::string ha2Hex = ha2HexStream.str();

    std::string finalInput = ha1Hex + ":" + nonce + ":" + ha2Hex;
    unsigned char finalDigest[16];
    md5(std::vector<uint8_t>(finalInput.begin(), finalInput.end()), finalDigest);

    std::ostringstream finalHexStream;
    for (unsigned char byte : finalDigest) {
        finalHexStream << std::setw(2) << std::setfill('0') << std::hex << (int)byte;
    }
    std::string expectedResponse = finalHexStream.str();

    return expectedResponse == response;
}

MessageHandler::MessageHandler(tcp::acceptor acceptor)
    : m_acceptor(std::move(acceptor)), m_threadpool(std::thread::hardware_concurrency()) {}

void MessageHandler::Start()
{
    acceptConnections();
}

void MessageHandler::acceptConnections()
{
    m_acceptor.async_accept([this](beast::error_code er, tcp::socket socket) {
        if (!er) {
            asio::post(m_threadpool, [this, socket = std::move(socket)]() mutable {
                readRequest(std::move(socket));
                });
        }
        else {
            std::cerr << "Accept error: " << er.message() << std::endl;
        }

        acceptConnections();
        });
}

void MessageHandler::readRequest(tcp::socket socket)
{
    beast::flat_buffer buffer;
    http::request_parser<http::string_body> parser;
    parser.body_limit(2 * 1024 * 1024);

    parser.body_limit(2 * 1024 * 1024);

    http::async_read(socket, buffer, parser,
        [this, &socket, &buffer, &parser](beast::error_code er, std::size_t b) mutable {
            if (!er) {
                try {
                    const auto& req = parser.get();

                    std::string requiredPassword = "default";
                    if (!checkDigest(req, requiredPassword)) {
                        http::response<http::string_body> res{ http::status::unauthorized, req.version() };
                        res.set(http::field::content_type, "text/plain");
                        res.body() = "Unauthorized";
                        res.prepare_payload();

                        http::async_write(socket, res,
                            [&socket](beast::error_code write_er, std::size_t) {
                                if (write_er) {
                                    std::cerr << "Write error: " << write_er.message() << std::endl;
                                }
                                //socket.shutdown(tcp::socket::shutdown_send);
                            });
                        return;
                    }

                    http::response<http::string_body> res{ http::status::ok, req.version() };
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Request processed successfully";
                    res.prepare_payload();

                    http::async_write(socket, res,
                        [&socket](beast::error_code write_er, std::size_t) {
                            if (write_er) {
                                std::cerr << "Write error: " << write_er.message() << std::endl;
                            }
                            //socket.shutdown(tcp::socket::shutdown_send);
                        });
                }
                catch (const std::exception& ex) {
                    std::cerr << "Request processing error: " << ex.what() << std::endl;
                    //socket.shutdown(tcp::socket::shutdown_both);
                }
            }
            else {
                std::cerr << "Read error: " << er.message() << " " << er.value() << std::endl;
                //socket.shutdown(tcp::socket::shutdown_both);
            }
        });
}









/*void MessageHandler::readRequest(tcp::socket socket)
{
    beast::flat_buffer buffer;
    std::cout << buffer.data().data();
    http::request_parser<http::string_body> parser;
    parser.body_limit(2 * 1024 * 1024);

    http::async_read(socket, buffer, parser,
        [this, socket = std::move(socket), &buffer, &parser](beast::error_code er, std::size_t) mutable {
            if (!er) {
                try {
                    const auto& req = parser.get();
                    std::ofstream log("log.txt", std::ios::app);
                    std::size_t fileSize = req.body().size();
                    std::size_t directorySize = ServerStorage::getStorageSize("./storage");

                    log << "[INFO] Incoming request with file size: " << fileSize << " bytes.\n";
                    log << "[INFO] Buffer content: " << buffer.size() << " bytes.\n";

                    if (directorySize + fileSize > MAX_SIZE) {
                        log << "[INFO] Storage size exceeded. Deleting old files.\n";
                        ServerStorage::deleteFiles("./storage", MAX_SIZE, DATE_FILE);
                    }

                    std::string filename = ServerStorage::generateFilename();

                    std::ofstream outFile(filename, std::ios::binary);
                    if (!outFile.is_open()) {
                        throw std::runtime_error("Failed to open file for writing.");
                    }

                    outFile.write(req.body().data(), req.body().size());
                    outFile.close();

                    log << "[SAVE] " << filename << " saved to storage.\n";
                    ServerStorage::addFile(std::filesystem::path(filename).filename().string(), DATE_FILE);

                    http::response<http::string_body> res{ http::status::ok, req.version() };
                    res.set(http::field::server, "AudioServer");
                    res.set(http::field::content_type, "text/plain");
                    res.body() = "Part of audio saved as " + filename;
                    res.prepare_payload();

                    log << "[INFO] Sending response with status: " << res.result_int() << ".\n";

                    http::async_write(socket, res, [](beast::error_code er, std::size_t) {
                        if (er) {
                            std::cerr << "Write error: " << er.message() << std::endl;
                        }
                        });
                }
                catch (const std::exception& ex) {
                    std::cerr << "Request processing error: " << ex.what() << std::endl;
                }
            }
            else {
                std::cerr << "Read error: " << er.message() << std::endl;
            }
        });
}*/






