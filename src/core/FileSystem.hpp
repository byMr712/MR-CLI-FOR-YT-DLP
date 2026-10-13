#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace mrcli::core {

class FileSystem {
public:
    [[nodiscard]] static bool fileExists(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static bool dirExists(const std::filesystem::path& path) noexcept;
    static bool createDirectories(const std::filesystem::path& path) noexcept;
    static bool removeFile(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static uintmax_t getFileSize(const std::filesystem::path& path) noexcept;

    static void openFolderInExplorer(const std::filesystem::path& path);
    static void openUrlInBrowser(std::string_view url);
};

} // namespace mrcli::core
