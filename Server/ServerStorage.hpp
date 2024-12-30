#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <map>

namespace ServerStorage { 
	 std::size_t getStorageSize(const std::string& directory);
	 std::string generateFilename(const std::string& sessionDir);
	 void deleteFiles(const std::string& directory, std::size_t maxSize, const std::string& file);
	 void addFile(const std::string& filename, const std::string& file, const std::string& sessionId);

	 std::map<std::string, std::time_t> readAssociations(const std::string& file);
	 void writeAssociations(const std::map<std::string, std::time_t>& associations, const std::string& file);

};
