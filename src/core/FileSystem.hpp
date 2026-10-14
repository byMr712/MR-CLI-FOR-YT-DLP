#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mrcli::core {

class FileSystem {
public:
    [[nodiscard]] static bool fileExists(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static bool dirExists(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static bool directoryExists(const std::filesystem::path& path) noexcept { return dirExists(path); }
    static bool createDirectories(const std::filesystem::path& path) noexcept;
    static bool removeFile(const std::filesystem::path& path) noexcept;
    static bool deleteFile(const std::filesystem::path& path) noexcept { return removeFile(path); }
    static bool copyFile(const std::filesystem::path& src, const std::filesystem::path& dst, bool overwrite = true) noexcept;
    static bool renameFile(const std::filesystem::path& src, const std::filesystem::path& dst) noexcept;
    [[nodiscard]] static uintmax_t getFileSize(const std::filesystem::path& path) noexcept;
    [[nodiscard]] static std::string getFileName(const std::filesystem::path& path);
    [[nodiscard]] static std::string getDirectoryName(const std::filesystem::path& path);
    [[nodiscard]] static std::vector<std::string> listFiles(const std::filesystem::path& dir);

    static void openFolderInExplorer(const std::filesystem::path& path);
    static void openUrlInBrowser(std::string_view url);
};

} // namespace mrcli::core
