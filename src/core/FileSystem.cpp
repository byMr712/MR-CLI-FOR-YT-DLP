#include "FileSystem.hpp"
#include "WinLibs.hpp"
#include "StringUtil.hpp"
#include <shellapi.h>

namespace fs = std::filesystem;

namespace mrcli::core {

bool FileSystem::fileExists(const fs::path& path) noexcept {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool FileSystem::dirExists(const fs::path& path) noexcept {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool FileSystem::createDirectories(const fs::path& path) noexcept {
    std::error_code ec;
    return fs::create_directories(path, ec);
}

bool FileSystem::removeFile(const fs::path& path) noexcept {
    std::error_code ec;
    return fs::remove(path, ec);
}

bool FileSystem::copyFile(const fs::path& src, const fs::path& dst, bool overwrite) noexcept {
    std::error_code ec;
    auto options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
    return fs::copy_file(src, dst, options, ec);
}

bool FileSystem::renameFile(const fs::path& src, const fs::path& dst) noexcept {
    std::error_code ec;
    fs::rename(src, dst, ec);
    return !ec;
}

uintmax_t FileSystem::getFileSize(const fs::path& path) noexcept {
    std::error_code ec;
    auto sz = fs::file_size(path, ec);
    return ec ? 0 : sz;
}

std::string FileSystem::getFileName(const fs::path& path) {
    return path.filename().u8string();
}

std::string FileSystem::getDirectoryName(const fs::path& path) {
    return path.parent_path().u8string();
}

std::vector<std::string> FileSystem::listFiles(const fs::path& dir) {
    std::vector<std::string> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!ec && entry.is_regular_file(ec)) {
            files.push_back(entry.path().u8string());
        }
    }
    return files;
}

void FileSystem::openFolderInExplorer(const fs::path& path) {
    std::wstring wpath = path.wstring();
    ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

void FileSystem::openUrlInBrowser(std::string_view url) {
    std::wstring wurl = StringUtil::utf8ToWstring(url);
    ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

} // namespace mrcli::core
