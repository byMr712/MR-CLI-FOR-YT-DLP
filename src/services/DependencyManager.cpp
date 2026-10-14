#include "DependencyManager.hpp"
#include "src/core/FileSystem.hpp"
#include "src/core/HttpClient.hpp"
#include "src/core/ZipExtractor.hpp"
#include "src/core/StringUtil.hpp"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace mrcli::services {

DependencyManager::DependencyManager(std::shared_ptr<PathProvider> paths,
                                     std::shared_ptr<ILogger> logger,
                                     std::shared_ptr<LocalizationService> loc)
    : m_paths(std::move(paths)), m_logger(std::move(logger)), m_loc(std::move(loc)) {
    refreshStatus();
}

void DependencyManager::refreshStatus() {
    m_status.ytDlpPath = m_paths->getYtDlpPath();
    m_status.ytDlpInstalled = core::FileSystem::fileExists(m_status.ytDlpPath);

    m_status.ffmpegPath = m_paths->getFfmpegPath();
    m_status.ffmpegInstalled = core::FileSystem::fileExists(m_status.ffmpegPath);

    m_status.quickJsPath = m_paths->getQuickJsPath();
    m_status.quickJsInstalled = core::FileSystem::fileExists(m_status.quickJsPath);
}

const domain::DependencyStatus& DependencyManager::getStatus() const {
    return m_status;
}

bool DependencyManager::checkAndCopyFromPeerFFmpeg() {
    std::string peerDir = m_paths->getPeerFFmpegDir();
    if (peerDir.empty() || !core::FileSystem::fileExists(peerDir + "ffmpeg.exe")) {
        return false;
    }

    std::string configPath = m_paths->getConfigDir();
    if (!core::FileSystem::directoryExists(configPath)) {
        core::FileSystem::createDirectories(configPath);
    }

    std::vector<std::string> tools = { "ffmpeg.exe", "ffprobe.exe", "ffplay.exe" };
    bool copiedAny = false;

    for (const auto& file : tools) {
        std::string src = peerDir + file;
        std::string dst = configPath + file;
        if (core::FileSystem::fileExists(src)) {
            if (core::FileSystem::copyFile(src, dst, true)) {
                copiedAny = true;
                m_logger->info(m_loc->get(StrId::Copied) + file);
            }
        }
    }

    if (copiedAny && core::FileSystem::fileExists(configPath + "ffmpeg.exe")) {
        refreshStatus();
        return true;
    }
    return false;
}

void DependencyManager::organizeExtractedTool(const std::string& targetExe, const std::string& destDir) {
    std::error_code ec;
    fs::path targetDir = fs::u8path(destDir);

    fs::path foundExePath;
    for (const auto& entry : fs::recursive_directory_iterator(targetDir, fs::directory_options::skip_permission_denied, ec)) {
        if (!ec && entry.is_regular_file(ec)) {
            if (entry.path().filename().u8string() == targetExe || entry.path().filename().string() == targetExe) {
                if (entry.path().parent_path() != targetDir) {
                    foundExePath = entry.path();
                    break;
                }
            }
        }
    }

    if (foundExePath.empty()) {
        return;
    }

    fs::path binDir = foundExePath.parent_path();
    for (const auto& entry : fs::directory_iterator(binDir, ec)) {
        if (!ec && entry.is_regular_file(ec)) {
            fs::copy_file(entry.path(), targetDir / entry.path().filename(), fs::copy_options::overwrite_existing, ec);
        }
    }

    for (const auto& entry : fs::directory_iterator(targetDir, ec)) {
        if (!ec && entry.is_directory(ec)) {
            fs::remove_all(entry.path(), ec);
        }
    }
}

bool DependencyManager::downloadAndExtractTool(const std::string& name,
                                              const std::string& url,
                                              const std::string& targetExe,
                                              const std::string& label,
                                              bool isZip) {
    std::string configPath = m_paths->getConfigDir();
    core::FileSystem::createDirectories(configPath);

    core::HttpClient http;
    auto progressCb = [&](uint64_t downloaded, uint64_t total) {
        if (total > 0) {
            double pct = (static_cast<double>(downloaded) / static_cast<double>(total)) * 100.0;
            double curMB = static_cast<double>(downloaded) / 1048576.0;
            double totMB = static_cast<double>(total) / 1048576.0;
            char buf[64];
            snprintf(buf, sizeof(buf), "(%.1f / %.1f MB)", curMB, totMB);
            
            const int barWidth = 25;
            int pos = static_cast<int>(barWidth * pct / 100.0);
            std::cout << "\r[" << label << "] [";
            for (int i = 0; i < barWidth; ++i) {
                std::cout << (i < pos ? '=' : (i == pos ? '>' : ' '));
            }
            std::cout << "] " << static_cast<int>(pct) << "% " << buf << "        " << std::flush;
        }
    };

    if (!isZip) {
        std::string dest = configPath + targetExe;
        bool ok = http.downloadFile(url, dest, progressCb);
        std::cout << "\n";
        refreshStatus();
        return ok && core::FileSystem::fileExists(dest);
    }

    std::string zipFile = configPath + name + ".zip";
    bool ok = http.downloadFile(url, zipFile, progressCb);
    std::cout << "\n";
    if (!ok) {
        core::FileSystem::deleteFile(zipFile);
        return false;
    }

    m_logger->info(m_loc->get(StrId::ExtractingComponents));
    bool extracted = core::ZipExtractor::extractZip(zipFile, configPath);
    core::FileSystem::deleteFile(zipFile);

    if (extracted) {
        organizeExtractedTool(targetExe, configPath);
    }

    refreshStatus();
    return extracted && core::FileSystem::fileExists(configPath + targetExe);
}

bool DependencyManager::installYtDlp() {
    m_logger->info(m_loc->get(StrId::InstallingYtDlp));
    return downloadAndExtractTool(
        "yt-dlp",
        "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe",
        "yt-dlp.exe",
        "yt-dlp",
        false
    );
}

bool DependencyManager::installFFmpeg() {
    m_logger->info(m_loc->get(StrId::InstallingFFmpeg));
    return downloadAndExtractTool(
        "ffmpeg",
        "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip",
        "ffmpeg.exe",
        "FFmpeg",
        true
    );
}

bool DependencyManager::installQuickJs() {
    m_logger->info(m_loc->get(StrId::InstallingQuickJS));
    return downloadAndExtractTool(
        "quickjs",
        "https://bellard.org/quickjs/binary_releases/quickjs-win-x86_64-2026-06-04.zip",
        "qjs.exe",
        "QuickJS",
        true
    );
}

bool DependencyManager::ensureAllDependencies(bool allowAutoInstall) {
    refreshStatus();

    if (!m_status.ffmpegInstalled) {
        checkAndCopyFromPeerFFmpeg();
    }

    if (m_status.isComplete()) {
        return true;
    }

    if (!allowAutoInstall) {
        return m_status.ytDlpInstalled;
    }

    if (!m_status.ytDlpInstalled) {
        installYtDlp();
    }
    if (!m_status.ffmpegInstalled) {
        installFFmpeg();
    }
    if (!m_status.quickJsInstalled) {
        installQuickJs();
    }

    refreshStatus();
    return m_status.ytDlpInstalled;
}

} // namespace mrcli::services
