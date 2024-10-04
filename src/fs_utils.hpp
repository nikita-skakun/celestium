#pragma once
#include "logging.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <vector>

template <class CharContainer>
constexpr CharContainer ReadFromFile(const std::filesystem::path &filepath)
{
    if (!std::filesystem::exists(filepath))
        LogMessage(LogLevel::ERROR, std::format("File does not exist: {}", filepath.string()));

    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        LogMessage(LogLevel::ERROR, std::format("Unable to open file: {}", filepath.string()));

    auto fileSize = std::filesystem::file_size(filepath);
    CharContainer cc(fileSize);

    file.read(reinterpret_cast<char *>(cc.data()), fileSize);
    if (!file)
        LogMessage(LogLevel::ERROR, std::format("Error reading file: {}", filepath.string()));

    return cc;
}
