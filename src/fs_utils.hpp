#pragma once
#include "logging.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <vector>

constexpr std::uintmax_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100 MB limit

template <class CharContainer>
constexpr CharContainer ReadFromFile(const std::filesystem::path &filepath)
{
    if (!std::filesystem::exists(filepath))
        LogMessage(LogLevel::ERROR, std::format("File does not exist: {}", filepath.string()));

    auto fileSize = std::filesystem::file_size(filepath);
    if (fileSize > MAX_FILE_SIZE)
        LogMessage(LogLevel::ERROR, std::format("File size exceeds maximum allowed size: {}", filepath.string()));

    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        LogMessage(LogLevel::ERROR, std::format("Unable to open file: {}", filepath.string()));

    CharContainer cc(fileSize);

    file.read(reinterpret_cast<char *>(cc.data()), fileSize);
    if (!file)
        LogMessage(LogLevel::ERROR, std::format("Error reading file: {}", filepath.string()));

    return cc;
}
