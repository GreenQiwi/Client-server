#include "ServerStorage.hpp"

std::size_t ServerStorage::getStorageSize(const std::string& directory)
{
	std::size_t size = 0;
	for (const auto& file : std::filesystem::directory_iterator(directory))
	{
		size += std::filesystem::file_size(file);
	}

	return size;
}

std::string ServerStorage::generateFilename()
{
	auto now = std::chrono::system_clock::now();
	std::time_t time_now = std::chrono::system_clock::to_time_t(now);

	std::tm time_info;
	localtime_s(&time_info, &time_now);

	std::ostringstream oss;
	oss << "./storage/audio_part_"
		<< std::put_time(&time_info, "%Y%m%d%H%M%S")
		<< ".wav";

	return oss.str();
}

void ServerStorage::deleteFiles(const std::string& directory, std::size_t maxSize, std::ofstream& log)
{
    std::vector<std::filesystem::path> files;
    size_t size = 0;

    for (const auto& file : std::filesystem::directory_iterator(directory))
    {
        if (file.is_regular_file() && file.path().filename().string().find("audio_part_") == 0)
        {
            files.push_back(file.path());
            size += std::filesystem::file_size(file.path());
        }
    }

    if (size <= maxSize)
    {
        return;
    }

    std::sort(std::begin(files), std::end(files), [](std::filesystem::path a, std::filesystem::path b)
        {
            return a.filename().string() < b.filename().string();
        }
    );

    for (auto file : files)
    {
        size -= std::filesystem::file_size(file);
        std::filesystem::remove(file);
        log << file.filename() << " is deleted\n";

        if (size <= maxSize)
        {
            break;
        }
    }
}