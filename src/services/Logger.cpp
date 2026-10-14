#include "Logger.hpp"
#include "LocalizationService.hpp"
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

    bool isRu = (m_loc && m_loc->getLanguage() == domain::Language::Russian);

    switch (level) {
        case LogLevel::Debug:
            color = core::ConsoleColor::Blue;
            prefix = isRu ? "[ОТЛАДКА] " : "[DEBUG] ";
            break;
        case LogLevel::Info:
            color = core::ConsoleColor::Yellow;
            prefix = isRu ? "[ИНФО] " : "[INFO] ";
            break;
        case LogLevel::Warning:
            color = core::ConsoleColor::Yellow;
            prefix = isRu ? "[ПРЕДУПРЕЖДЕНИЕ] " : "[WARNING] ";
            break;
        case LogLevel::Error:
            color = core::ConsoleColor::Red;
            prefix = isRu ? "[ОШИБКА] " : "[ERROR] ";
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

    if (!checkView.empty() && checkView[0] == '[') {
        size_t closeBracket = checkView.find(']');
        if (closeBracket != std::string_view::npos && closeBracket <= 30) {
            prefix.clear();

            std::string_view tag = checkView.substr(0, closeBracket + 1);
            if (tag == "[OK]") {
                color = core::ConsoleColor::Green;
            } else if (tag == "[ERROR]" || tag == "[ОШИБКА]") {
                color = core::ConsoleColor::Red;
            } else if (tag == "[DEBUG]" || tag == "[ОТЛАДКА]") {
                color = core::ConsoleColor::Blue;
            } else if (tag == "[WARNING]" || tag == "[ПРЕДУПРЕЖДЕНИЕ]") {
                color = core::ConsoleColor::Yellow;
            } else if (tag == "[INFO]" || tag == "[ИНФО]") {
                color = (level == LogLevel::Success) ? core::ConsoleColor::Green : core::ConsoleColor::Yellow;
            } else if (tag == "[Merger]" || tag == "[Слияние]") {
                color = core::ConsoleColor::Cyan;
            }
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
