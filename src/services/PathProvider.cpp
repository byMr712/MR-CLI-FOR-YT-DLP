#include "PathProvider.hpp"
#include "../core/WinLibs.hpp"
#include "../core/FileSystem.hpp"
#include "../core/StringUtil.hpp"

namespace mrcli::services {

PathProvider::PathProvider() {
    wchar_t docPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, 0, docPath))) {
        m_documentsDir = docPath;
    } else {
        m_documentsDir = L"C:";
    }

    m_appDir = m_documentsDir / L"MR-CLI-FOR-YT-DLP";
    m_configDir = m_appDir / L"configs";
    m_downloadDir = m_appDir / L"downloads";
    m_pluginsDir = m_configDir / L"yt-dlp-plugins";
}

std::filesystem::path PathProvider::getPeerFfmpegExePath() const noexcept {
    std::filesystem::path peer = m_documentsDir / L"MR-CLI-FOR-FFMPEG" / L"configs" / L"ffmpeg.exe";
    if (core::FileSystem::fileExists(peer)) {
        return peer;
    }
    std::filesystem::path fallback = L"C:\\MR-CLI-FOR-FFMPEG\\configs\\ffmpeg.exe";
    if (core::FileSystem::fileExists(fallback)) {
        return fallback;
    }
    return {};
}

void PathProvider::ensureDirectoriesExist() const {
    core::FileSystem::createDirectories(m_appDir);
    core::FileSystem::createDirectories(m_configDir);
    core::FileSystem::createDirectories(m_downloadDir);
    core::FileSystem::createDirectories(m_pluginsDir);
}

} // namespace mrcli::services
