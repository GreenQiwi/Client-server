#include "Authentication.hpp"

Authentication::Authentication()
    : m_username(""), m_password(""), m_token(""), m_authHeader("")
    {
    std::ifstream file("client_id.txt");
    if (file) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        m_token = buffer.str();
    }
}

void Authentication::Authenticate(http::response<http::string_body> res) {
    try {            
        auto authHeader = res[http::field::www_authenticate];
        std::string realm, nonce;

        auto realmPos = authHeader.find("realm=\"");
        if (realmPos != std::string::npos) {
            realmPos += 7;
            auto end = authHeader.find("\"", realmPos);
            realm = authHeader.substr(realmPos, end - realmPos);
        }

        auto noncePos = authHeader.find("nonce=\"");
        if (noncePos != std::string::npos) {
            noncePos += 11;  
            auto end = authHeader.find("\"", noncePos);
            nonce = authHeader.substr(noncePos, end - noncePos);
        }

        if (realm.empty() || nonce.empty()) {
            throw std::runtime_error("Failed to extract realm or nonce from WWW-Authenticate header.");
        }

        std::string cnonce = generateNonce();
        std::string nc = "00000001";
        std::string qop = "auth";

        std::string response = generateDigest("POST", "/audioserver", nonce, realm, cnonce, nc, qop);

        m_authHeader = "Digest username=\"" + m_username +
            "\", realm=\"" + realm +
            "\", nonce=\"" + nonce +
            "\", uri=\"/audioserver\", algorithm=MD5, qop=\"" + qop +
            "\", nc=" + nc +
            ", cnonce=\"" + cnonce +
            "\", response=\"" + response + "\"";

    }
    catch (const std::exception& ex) {
        std::cerr << "Authentication failed: " << ex.what() << std::endl;
    }
}

std::string Authentication::GetAuthHeader() { 
    return m_authHeader; 
}

void Authentication::SetUsername(std::string username) { 
    m_username = username; 
}

void Authentication::SetPassword(std::string password) { 
    m_password = password; 
}

std::string Authentication::GetUsername() { 
    return m_username; 
}

std::string Authentication::GetToken() { 
    return m_token; 
}

void Authentication::SetToken(const std::string& token) { 
    m_token = token; 
}

void Authentication::LogIn()
{
    std::string username, password;

    std::cout << std::endl;
    std::cout << "Enter login: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;
    std::cout << std::endl;

    SetUsername(username);
    SetPassword(password);
}

std::string Authentication::generateDigest(const std::string& method, const std::string& uri,
    const std::string& nonce, const std::string& realm,
    const std::string& cnonce, const std::string& nc, const std::string& qop) {

    std::string ha1 = calculateMD5(m_username + ":" + realm + ":" + m_password);
    std::string ha2 = calculateMD5(method + ":" + uri);

    std::string digest = calculateMD5(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2);
    return digest;
}


std::string Authentication::GenerateHa1(std::string realm) {
    return calculateMD5(m_username + ":" + realm + ":" + m_password);
}

std::string Authentication::calculateMD5(const std::string& input) {
    std::vector<uint8_t> inputVec(input.begin(), input.end());
    uint8_t digest[16];
    md5(inputVec, digest);

    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return oss.str();
}

uint32_t Authentication::left_rotate(uint32_t x, uint32_t c) {
    return (x << c) | (x >> (32 - c));
}

void Authentication::md5(const std::vector<uint8_t>& input, uint8_t digest[16]) {
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

std::string Authentication::generateNonce() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}
