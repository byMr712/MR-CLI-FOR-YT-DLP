#pragma once
#include <filesystem>

namespace mrcli::services {

class PathProvider {
public:
    PathProvider();

    [[nodiscard]] std::filesystem::path getAppDirectory() const noexcept { return m_appDir; }
    [[nodiscard]] std::filesystem::path getConfigDirectory() const noexcept { return m_configDir; }
    [[nodiscard]] std::filesystem::path getDefaultDownloadDirectory() const noexcept { return m_downloadDir; }
    [[nodiscard]] std::filesystem::path getPluginsDirectory() const noexcept { return m_pluginsDir; }

    [[nodiscard]] std::filesystem::path getConfigFile() const noexcept { return m_configDir / "mr-config.txt"; }
    [[nodiscard]] std::filesystem::path getArchiveFile() const noexcept { return m_configDir / "archive.txt"; }
    [[nodiscard]] std::filesystem::path getCookiesEncryptedFile() const noexcept { return m_configDir / "cookies.dat"; }
    [[nodiscard]] std::filesystem::path getLegacyCookiesFile() const noexcept { return m_configDir / "cookies.txt"; }

    [[nodiscard]] std::filesystem::path getYtDlpExePath() const noexcept { return m_configDir / "yt-dlp.exe"; }
    [[nodiscard]] std::filesystem::path getFfmpegExePath() const noexcept { return m_configDir / "ffmpeg.exe"; }
    [[nodiscard]] std::filesystem::path getQjsExePath() const noexcept { return m_configDir / "qjs.exe"; }

    [[nodiscard]] std::filesystem::path getPeerFfmpegExePath() const noexcept;

    void ensureDirectoriesExist() const;

private:
    std::filesystem::path m_documentsDir;
    std::filesystem::path m_appDir;
    std::filesystem::path m_configDir;
    std::filesystem::path m_downloadDir;
    std::filesystem::path m_pluginsDir;
};

} // namespace mrcli::services
