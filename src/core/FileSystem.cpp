#include "FileSystem.hpp"
#include "WinLibs.hpp"
#include "StringUtil.hpp"

namespace mrcli::core {

bool FileSystem::fileExists(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool FileSystem::dirExists(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool FileSystem::createDirectories(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec);
}

bool FileSystem::removeFile(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

uintmax_t FileSystem::getFileSize(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    return std::filesystem::file_size(path, ec);
}

void FileSystem::openFolderInExplorer(const std::filesystem::path& path) {
    std::wstring wpath = path.wstring();
    ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

void FileSystem::openUrlInBrowser(std::string_view url) {
    std::wstring wurl = StringUtil::utf8ToWstring(url);
    ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

} // namespace mrcli::core
