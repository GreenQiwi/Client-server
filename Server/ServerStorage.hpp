#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <map>

namespace ServerStorage { 
	 std::size_t GetStorageSize(const std::string& directory);
	 std::string GenerateFilename(const std::string& sessionDir);
	 void DeleteFiles(const std::string& directory, const int maxSize, const std::string& file);
	 void AddFile(const std::string& filename, const std::string& file, const std::string& sessionId);

	 std::map<std::string, std::time_t> ReadAssociations(const std::string& file);
	 void WriteAssociations(const std::map<std::string, std::time_t>& associations, const std::string& file);

};
