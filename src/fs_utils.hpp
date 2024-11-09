#pragma once
#include <filesystem>
#include <format>
#include <fstream>

constexpr std::uintmax_t MAX_FILE_SIZE = 100 * 1024 * 1024; // 100 MB limit

template <class CharContainer>
constexpr CharContainer ReadFromFile(const std::filesystem::path &filepath)
{
    if (!std::filesystem::exists(filepath))
        throw std::runtime_error(std::format("File does not exist: {}", filepath.string()));

    auto fileSize = std::filesystem::file_size(filepath);
    if (fileSize > MAX_FILE_SIZE)
        throw std::runtime_error(std::format("File size exceeds maximum allowed size: {}", filepath.string()));

    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    file.open(filepath, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error(std::format("Unable to open file: {}", filepath.string()));

    CharContainer cc(fileSize);

    try
    {
        file.read(cc.data(), fileSize);
    }
    catch (const std::ios_base::failure &e)
    {
        throw std::runtime_error(std::format("Error reading file {}: {}", filepath.string(), e.what()));
    }

    return cc;
}
