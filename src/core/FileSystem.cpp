#include "FileSystem.hpp"
#include "WinLibs.hpp"
#include "StringUtil.hpp"
#include <shellapi.h>

namespace fs = std::filesystem;

namespace mrcli::core {

bool FileSystem::fileExists(const PathArg& path) noexcept {
    std::error_code ec;
    return fs::is_regular_file(path.path(), ec);
}

bool FileSystem::dirExists(const PathArg& path) noexcept {
    std::error_code ec;
    return fs::is_directory(path.path(), ec);
}

bool FileSystem::createDirectories(const PathArg& path) noexcept {
    std::error_code ec;
    return fs::create_directories(path.path(), ec);
}

bool FileSystem::removeFile(const PathArg& path) noexcept {
    std::error_code ec;
    return fs::remove(path.path(), ec);
}

bool FileSystem::copyFile(const PathArg& src, const PathArg& dst, bool overwrite) noexcept {
    std::error_code ec;
    auto options = overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none;
    return fs::copy_file(src.path(), dst.path(), options, ec);
}

bool FileSystem::renameFile(const PathArg& src, const PathArg& dst) noexcept {
    std::error_code ec;
    fs::rename(src.path(), dst.path(), ec);
    return !ec;
}

uintmax_t FileSystem::getFileSize(const PathArg& path) noexcept {
    std::error_code ec;
    auto sz = fs::file_size(path.path(), ec);
    return ec ? 0 : sz;
}

std::string FileSystem::getFileName(const PathArg& path) {
    return path.path().filename().u8string();
}

std::string FileSystem::getDirectoryName(const PathArg& path) {
    return path.path().parent_path().u8string();
}

std::vector<std::string> FileSystem::listFiles(const PathArg& dir) {
    std::vector<std::string> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir.path(), ec)) {
        if (!ec && entry.is_regular_file(ec)) {
            files.push_back(entry.path().u8string());
        }
    }
    return files;
}

void FileSystem::openFolderInExplorer(const PathArg& path) {
    std::wstring wpath = path.path().wstring();
    ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

void FileSystem::openUrlInBrowser(std::string_view url) {
    std::wstring wurl = StringUtil::utf8ToWstring(url);
    ShellExecuteW(nullptr, L"open", wurl.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
}

} // namespace mrcli::core
