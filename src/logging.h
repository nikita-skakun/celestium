#pragma once
#include <string>
#include <iostream>
#include <termcolor/termcolor.hpp>

// Enum for log levels
enum class LogLevel
{
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

// Function to log messages with colors
void LogMessage(LogLevel level, const std::string &message);
