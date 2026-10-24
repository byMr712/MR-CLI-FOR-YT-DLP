#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mrcli::core {

class PathArg {
public:
    PathArg(const std::filesystem::path& p) : m_path(p) {}
    PathArg(std::filesystem::path&& p) noexcept : m_path(std::move(p)) {}
    PathArg(const std::string& s) : m_path(std::filesystem::u8path(s)) {}
    PathArg(std::string_view s) : m_path(std::filesystem::u8path(s)) {}
    PathArg(const char* s) : m_path(std::filesystem::u8path(s ? s : "")) {}
    PathArg(const std::wstring& ws) : m_path(ws) {}
    PathArg(const wchar_t* ws) : m_path(ws ? ws : L"") {}

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return m_path; }
    operator const std::filesystem::path&() const noexcept { return m_path; }

private:
    std::filesystem::path m_path;
};

class FileSystem {
public:
    [[nodiscard]] static bool fileExists(const PathArg& path) noexcept;
    [[nodiscard]] static bool dirExists(const PathArg& path) noexcept;
    [[nodiscard]] static bool directoryExists(const PathArg& path) noexcept { return dirExists(path); }
    static bool createDirectories(const PathArg& path) noexcept;
    static bool removeFile(const PathArg& path) noexcept;
    static bool deleteFile(const PathArg& path) noexcept { return removeFile(path); }
    static bool copyFile(const PathArg& src, const PathArg& dst, bool overwrite = true) noexcept;
    static bool renameFile(const PathArg& src, const PathArg& dst) noexcept;
    [[nodiscard]] static uintmax_t getFileSize(const PathArg& path) noexcept;
    [[nodiscard]] static std::string getFileName(const PathArg& path);
    [[nodiscard]] static std::string getDirectoryName(const PathArg& path);
    [[nodiscard]] static std::vector<std::string> listFiles(const PathArg& dir);

    static void openFolderInExplorer(const PathArg& path);
    static void openUrlInBrowser(std::string_view url);
};

} // namespace mrcli::core
