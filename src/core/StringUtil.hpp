#pragma once
#include "WinLibs.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

namespace mrcli::core {

class StringUtil {
public:
    [[nodiscard]] static std::string wstringToUtf8(std::wstring_view wstr) {
        if (wstr.empty()) return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string result(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), size, nullptr, nullptr);
        return result;
    }

    [[nodiscard]] static std::wstring utf8ToWstring(std::string_view str) {
        if (str.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        if (size <= 0) return {};
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);
        return result;
    }

    [[nodiscard]] static std::string trim(std::string_view s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) return {};
        size_t end = s.find_last_not_of(" \t\r\n");
        return std::string(s.substr(start, end - start + 1));
    }

    [[nodiscard]] static bool startsWith(std::string_view str, std::string_view prefix) noexcept {
        return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
    }

    [[nodiscard]] static bool endsWith(std::string_view str, std::string_view suffix) noexcept {
        return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
    }

    [[nodiscard]] static std::string toLower(std::string_view str) {
        std::string result(str);
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return result;
    }

    [[nodiscard]] static std::vector<std::string> split(std::string_view str, char delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = str.find(delimiter);
        while (end != std::string_view::npos) {
            tokens.emplace_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        tokens.emplace_back(str.substr(start));
        return tokens;
    }
};

} // namespace mrcli::core
