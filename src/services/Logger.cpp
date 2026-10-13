#include "Logger.hpp"
#include "../core/Console.hpp"
#include <iostream>

namespace mrcli::services {

void ConsoleLogger::log(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    core::ConsoleColor color = core::ConsoleColor::White;
    std::string prefix;

    switch (level) {
        case LogLevel::Debug:
            color = core::ConsoleColor::Blue;
            prefix = "[DEBUG] ";
            break;
        case LogLevel::Info:
            color = core::ConsoleColor::Yellow;
            prefix = "[INFO] ";
            break;
        case LogLevel::Warning:
            color = core::ConsoleColor::Yellow;
            prefix = "[WARNING] ";
            break;
        case LogLevel::Error:
            color = core::ConsoleColor::Red;
            prefix = "[ERROR] ";
            break;
        case LogLevel::Success:
            color = core::ConsoleColor::Green;
            prefix = "[OK] ";
            break;
    }

    core::Console::printColored(prefix + std::string(message), color, true);
}

void ConsoleLogger::info(std::string_view message) {
    log(LogLevel::Info, message);
}

void ConsoleLogger::warn(std::string_view message) {
    log(LogLevel::Warning, message);
}

void ConsoleLogger::error(std::string_view message) {
    log(LogLevel::Error, message);
}

void ConsoleLogger::success(std::string_view message) {
    log(LogLevel::Success, message);
}

} // namespace mrcli::services
