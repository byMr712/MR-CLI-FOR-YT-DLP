#pragma once
#include <filesystem>
#include <string>

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

    // String convenience getters
    [[nodiscard]] std::string getConfigDir() const { return (m_configDir.u8string() + "\\"); }
    [[nodiscard]] std::string getDownloadsDir() const { return (m_downloadDir.u8string() + "\\"); }
    [[nodiscard]] std::string getPluginsDir() const { return (m_pluginsDir.u8string() + "\\"); }
    [[nodiscard]] std::string getCookiesFile() const { return (m_configDir / "cookies.txt").u8string(); }
    [[nodiscard]] std::string getEncryptedCookiesFile() const { return (m_configDir / "cookies.dat").u8string(); }
    [[nodiscard]] std::string getYtDlpPath() const { return (m_configDir / "yt-dlp.exe").u8string(); }
    [[nodiscard]] std::string getFfmpegPath() const { return (m_configDir / "ffmpeg.exe").u8string(); }
    [[nodiscard]] std::string getQuickJsPath() const { return (m_configDir / "qjs.exe").u8string(); }
    [[nodiscard]] std::string getPeerFFmpegDir() const {
        auto p = getPeerFfmpegExePath();
        if (p.empty()) return "";
        return (p.parent_path().u8string() + "\\");
    }

    void ensureDirectoriesExist() const;

private:
    std::filesystem::path m_documentsDir;
    std::filesystem::path m_appDir;
    std::filesystem::path m_configDir;
    std::filesystem::path m_downloadDir;
    std::filesystem::path m_pluginsDir;
};

} // namespace mrcli::services
