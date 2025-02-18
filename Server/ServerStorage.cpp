#include "ServerStorage.hpp"

const int MAX_SIZE = 10 * 1024 * 1024;

std::size_t ServerStorage::GetStorageSize(const std::string& directory)
{
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
    }

	std::size_t size = 0;
	for (const auto& file : std::filesystem::directory_iterator(directory))
	{
		size += std::filesystem::file_size(file);
	}

	return size;
}

std::string ServerStorage::GenerateFilename(const std::string& sessionDir)
{
    std::filesystem::path dir = "./storage/" + sessionDir;

    const auto& now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);

    std::tm time_info;
    localtime_s(&time_info, &time_now);

    std::ostringstream oss;
    oss << dir.string() + "/audio_part_"
        << std::put_time(&time_info, "%Y.%m.%d.%H.%M.%S")
        << ".wav";

    return oss.str();
}


void ServerStorage::AddFile(const std::string& filename, const std::string& file, const std::string& sessionDir)
{
    std::ofstream out(file, std::ios::app);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open associations file.");
    }

    auto now = std::chrono::system_clock::now();
    std::time_t time_now = std::chrono::system_clock::to_time_t(now);

    std::ofstream log("log.txt", std::ios::app);
    if (log.is_open()) {
        log << "[ADD] " << filename << " added with timestamp " << time_now << "\n";
    }

    out << sessionDir + "/" + filename << " " << time_now << "\n";
    out.close();
}

std::map<std::string, std::time_t> ServerStorage::ReadAssociations(const std::string& file)
{
    std::map<std::string, std::time_t> associations;

    std::ifstream in(file);
    if (!in.is_open()) 
    {
        throw std::runtime_error("Failed to open associations file.");
    }

    std::string filename;
    std::time_t timestamp;

    while (in >> filename >> timestamp) {
        associations[filename] = timestamp;
    }

    in.close();
    return associations;
}

void ServerStorage::WriteAssociations(const std::map<std::string, std::time_t>& associations, const std::string& file)
{
    std::ofstream out(file);
    if (!out.is_open()) 
    {
        throw std::runtime_error("Failed to open associations file.");
    }

    for (const auto& [filename, timestamp] : associations) {
        out << filename << " " << timestamp << "\n";
    }

    out.close();
}

void ServerStorage::DeleteFiles(const std::string& directory, const int maxSize, const std::string& file)
{
    std::map<std::string, std::time_t> associations = ReadAssociations(file);

    std::size_t size = GetStorageSize(directory);

    if (size <= maxSize) {
        return; 
    }   

    std::ofstream log("log.txt", std::ios::app);
    if (!log.is_open()) {
        throw std::runtime_error("Failed to open log file.");
    }

    for (const auto& [filename, timestamp] : associations) {
        std::filesystem::path filePath = "./storage/" + filename;

        if (std::filesystem::exists(filePath)) {
            std::size_t fileSize = std::filesystem::file_size(filePath);

            std::filesystem::remove(filePath);
            size -= fileSize;

            log << "[DELETE] " << filename << " (" << fileSize << " bytes) deleted to maintain storage size.\n";

            associations.erase(filename);

            if (size <= maxSize) {
                break;
            }
        }
    }

    WriteAssociations(associations, file);
    log << "[INFO] Date associations updated.\n";
}