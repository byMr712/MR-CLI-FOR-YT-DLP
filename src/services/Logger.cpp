#include "Logger.hpp"
#include "../core/Console.hpp"
#include <iostream>

namespace mrcli::services {

void ConsoleLogger::log(LogLevel level, std::string_view message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::string str(message);
    size_t start = 0;
    while (start < str.size() && (str[start] == '\n' || str[start] == '\r')) {
        start++;
    }
    std::string leadingNewlines = str.substr(0, start);
    std::string trimmedMsg = str.substr(start);

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

    size_t contentStart = 0;
    while (contentStart < trimmedMsg.size() && trimmedMsg[contentStart] == ' ') {
        contentStart++;
    }
    std::string_view checkView = std::string_view(trimmedMsg).substr(contentStart);

    auto hasPrefix = [](std::string_view s, std::string_view p) {
        return s.rfind(p, 0) == 0;
    };

    if (hasPrefix(checkView, "[INFO]") || hasPrefix(checkView, "[ИНФО]") ||
        hasPrefix(checkView, "[WARNING]") || hasPrefix(checkView, "[ПРЕДУПРЕЖДЕНИЕ]") ||
        hasPrefix(checkView, "[ERROR]") || hasPrefix(checkView, "[ОШИБКА]") ||
        hasPrefix(checkView, "[OK]") ||
        hasPrefix(checkView, "[DEBUG]") || hasPrefix(checkView, "[ОТЛАДКА]")) {

        prefix.clear();

        if (hasPrefix(checkView, "[OK]")) {
            color = core::ConsoleColor::Green;
        } else if (hasPrefix(checkView, "[ERROR]") || hasPrefix(checkView, "[ОШИБКА]")) {
            color = core::ConsoleColor::Red;
        } else if (hasPrefix(checkView, "[DEBUG]") || hasPrefix(checkView, "[ОТЛАДКА]")) {
            color = core::ConsoleColor::Blue;
        } else if (hasPrefix(checkView, "[WARNING]") || hasPrefix(checkView, "[ПРЕДУПРЕЖДЕНИЕ]")) {
            color = core::ConsoleColor::Yellow;
        } else if (hasPrefix(checkView, "[INFO]") || hasPrefix(checkView, "[ИНФО]")) {
            color = (level == LogLevel::Success) ? core::ConsoleColor::Green : core::ConsoleColor::Yellow;
        }
    }

    if (!leadingNewlines.empty()) {
        std::cout << leadingNewlines;
    }

    core::Console::printColored(prefix + trimmedMsg, color, true);
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
