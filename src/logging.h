#pragma once
#include <string>
#include <iostream>
#include <termcolor/termcolor.hpp>

// Enum for log levels
enum class LogLevel : u_int8_t
{
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

/**
 * Logs a message to the console with a specific log level, formatting the output color based on the level.
 *
 * @param level The severity level of the log message (INFO, WARNING, ERROR, DEBUG).
 * @param message The message to be logged.
 */
constexpr void LogMessage(LogLevel level, const std::string &message)
{
    if (message.empty())
        return;

    switch (level)
    {
    case LogLevel::INFO:
        std::cout << termcolor::green << "[INFO] " << termcolor::reset;
        break;
    case LogLevel::WARNING:
        std::cout << termcolor::yellow << "[WARNING] " << termcolor::reset;
        break;
    case LogLevel::ERROR:
        std::cout << termcolor::red << "[ERROR] " << termcolor::reset;
        break;
    case LogLevel::DEBUG:
        std::cout << termcolor::cyan << "[DEBUG] " << termcolor::reset;
        break;
    }
    std::cout << message << std::endl;
}
