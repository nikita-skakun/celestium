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

    std::uintmax_t fileSize = std::filesystem::file_size(filepath);
    if (fileSize > MAX_FILE_SIZE)
        LogMessage(LogLevel::ERROR, std::format("File size exceeds maximum allowed size: {}", filepath.string()));

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
        LogMessage(LogLevel::ERROR, std::format("Unable to open file: {}", filepath.string()));

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    CharContainer cc(fileSize);

    try
    {
        file.read(cc.data(), fileSize);
    }
    catch (const std::ifstream::failure &e)
    {
        LogMessage(LogLevel::ERROR, std::format("Error reading file {}: {}", filepath.string(), e.what()));
    }

    return cc;
}
