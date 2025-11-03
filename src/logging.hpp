#pragma once
#include <iostream>

// Enum for log levels
enum class LogLevel : uint8_t
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

/**
 * Logs a message to the console an exception depending on log level.
 *
 * @param level The severity level of the log message (DEBUG, INFO, WARNING, ERROR).
 * @param message The message to be logged.
 */
constexpr void LogMessage(LogLevel level, const std::string &message) noexcept
{
    if (message.empty())
        return;

    switch (level)
    {
    case LogLevel::DEBUG:
        std::cout << "\033[36m[DEBUG] \033[0m" << message << '\n';
        break;
    case LogLevel::INFO:
        std::cout << "\033[32m[INFO] \033[0m" << message << '\n';
        break;
    case LogLevel::WARNING:
        std::cout << "\033[33m[WARNING] \033[0m" << message << '\n';
        break;
    case LogLevel::ERROR:
        std::cout << "\033[31m[ERROR] \033[0m" << message << '\n';
        break;
    }
}
