#pragma once
#include <format>
#include <termcolor/termcolor.hpp>

// Enum for log levels
enum class LogLevel : u_int8_t
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
        std::cout << termcolor::cyan << "[DEBUG] " << termcolor::reset << message << std::endl;
        break;
    case LogLevel::INFO:
        std::cout << termcolor::green << "[INFO] " << termcolor::reset << message << std::endl;
        break;
    case LogLevel::WARNING:
        std::cout << termcolor::yellow << "[WARNING] " << termcolor::reset << message << std::endl;
        break;
    case LogLevel::ERROR:
        std::cout << termcolor::red << "[ERROR] " << termcolor::reset << message << std::endl;
        break;
    }
}
