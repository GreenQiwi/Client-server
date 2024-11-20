#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

class ServerStorage {
public:
	static std::size_t getStorageSize(const std::string& directory);
	static std::string generateFilename();
	static void deleteFiles(const std::string& directory, std::size_t maxSize, std::ofstream& log);
};