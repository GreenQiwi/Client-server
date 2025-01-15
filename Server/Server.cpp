#include "MessageHandler.hpp"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <string>
#include <unordered_set>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

void addUser(const std::string& username, const std::string& password, std::mutex& fileMutex) {
    const std::string userFile = "users.txt";
    std::unordered_set<std::string> existingUsers;

    {
        std::lock_guard<std::mutex> lock(fileMutex);

        std::ifstream inputFile(userFile);
        std::string line;
        while (std::getline(inputFile, line)) {
            std::istringstream iss(line);
            std::string user;    
            if (std::getline(iss, user, ':')) {
                existingUsers.insert(user);
            }
        }
        inputFile.close();

        if (existingUsers.find(username) != existingUsers.end()) {
            std::cerr << "[ERROR] User already exists: " << username << std::endl;
            return;
        }

        std::ofstream outputFile(userFile, std::ios::app);
        if (!outputFile.is_open()) {
            throw std::runtime_error("Failed to open user file for writing.");
        }
        outputFile << username << ":" << password << "\n";
        outputFile.close();
    }

    std::cout << "[INFO] User added: " << username << std::endl;
}

void commandHandler(std::mutex& fileMutex) {
    while (true) {
        std::string command;
        std::cout << "Enter command (add to add user, quit to exit): ";
        std::cin >> command;

        if (command == "add") {
            std::string username, password;
            std::cout << "Enter username: ";
            std::cin >> username;
            std::cout << "Enter password: ";
            std::cin >> password;
            addUser(username, password, fileMutex);
        }
        else if (command == "quit") {
            std::cout << "Exiting command handler..." << std::endl;
            break;
        }
        else {
            std::cout << "Unknown command." << std::endl;
        }
    }
}


int main() {
    setlocale(LC_ALL, "rus");

    try {
        auto ioc = std::make_shared<asio::io_context>(std::thread::hardware_concurrency());
        auto handler = std::make_shared<MessageHandler>(ioc);
        handler->Start();

        std::mutex fileMutex;

        std::thread commandThread(commandHandler, std::ref(fileMutex));

        std::vector<std::thread> threads;
        threads.reserve(std::thread::hardware_concurrency() - 1);
        for (std::size_t i = 0; i < std::thread::hardware_concurrency() - 1; ++i) {
            threads.emplace_back([ioc]() {
                ioc->run();
                });
        }

        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        if (commandThread.joinable()) {
            commandThread.join();
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }

    return 0;
}

