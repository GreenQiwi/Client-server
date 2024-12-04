#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <map>

class ServerStorage {
public:
	static std::size_t getStorageSize(const std::string& directory);
	static std::string generateFilename(const std::string& sessionDir);
	static void deleteFiles(const std::string& directory, std::size_t maxSize, const std::string& file);
	static void addFile(const std::string& filename, const std::string& file, const std::string& sessionId);
private:
	static std::map<std::string, std::time_t> readAssociations(const std::string& file);
	static void writeAssociations(const std::map<std::string, std::time_t>& associations, const std::string& file);
};