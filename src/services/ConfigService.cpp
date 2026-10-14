#include "ConfigService.hpp"
#include "../core/FileSystem.hpp"
#include "../core/StringUtil.hpp"
#include <fstream>

namespace mrcli::services {

ConfigService::ConfigService(std::filesystem::path configFilePath)
    : m_configFilePath(std::move(configFilePath)) {}

domain::AppConfig ConfigService::load() {
    domain::AppConfig config;

    if (core::FileSystem::fileExists(m_configFilePath)) {
        std::ifstream f(m_configFilePath, std::ios::binary);
        if (f.is_open()) {
            std::string line;
            while (std::getline(f, line)) {
                if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
                    static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
                    line = line.substr(3);
                }
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                    line.pop_back();
                }

                if (line.find("DOWNLOAD_PATH=") == 0) config.downloadPath = line.substr(14);
                else if (line.find("COOKIES_FILE=") == 0) config.cookiesFile = line.substr(13);
                else if (line.find("USE_COOKIES=") == 0) config.useCookies = (line.substr(12) == "true");
                else if (line.find("VIDEO_RESOLUTION=") == 0) config.videoResolution = line.substr(17);
                else if (line.find("VIDEO_FPS=") == 0) config.videoFps = line.substr(10);
                else if (line.find("VIDEO_FORMAT=") == 0) config.videoFormat = line.substr(13);
                else if (line.find("AUDIO_FORMAT=") == 0) config.audioFormat = line.substr(13);
                else if (line.find("CODEC_RECOMPILER=") == 0) config.codecRecompiler = (line.substr(17) == "true");
                else if (line.find("USE_ARCHIVE=") == 0) config.useArchive = (line.substr(12) == "true");
                else if (line.find("ONLY_AUDIO=") == 0) config.onlyAudio = (line.substr(11) == "true");
                else if (line.find("ONLY_VIDEO=") == 0) config.onlyVideo = (line.substr(11) == "true");
                else if (line.find("PLAYLIST_DELAY_ENABLED=") == 0) config.playlistDelayEnabled = (line.substr(23) == "true");
                else if (line.find("PLAYLIST_DELAY_INTERVAL=") == 0) {
                    try {
                        int v = std::stoi(line.substr(24));
                        if (v > 0) config.playlistDelayInterval = v;
                    } catch (...) {}
                }
                else if (line.find("PLAYLIST_DELAY_DURATION=") == 0) {
                    try {
                        int v = std::stoi(line.substr(24));
                        if (v > 0) config.playlistDelayDuration = v;
                    } catch (...) {}
                }
                else if (line.find("LANGUAGE=") == 0) {
                    config.language = (line.substr(9) == "ru") ? domain::Language::Russian : domain::Language::English;
                }
            }
        }
    }

    return config;
}

bool ConfigService::save(const domain::AppConfig& config) {
    core::FileSystem::createDirectories(m_configFilePath.parent_path());

    std::ofstream f(m_configFilePath, std::ios::out | std::ios::binary);
    if (!f.is_open()) return false;

    unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    f.write(reinterpret_cast<char*>(bom), sizeof(bom));

    f << "DOWNLOAD_PATH=" << config.downloadPath << "\n"
      << "COOKIES_FILE=" << config.cookiesFile << "\n"
      << "USE_COOKIES=" << (config.useCookies ? "true" : "false") << "\n"
      << "VIDEO_RESOLUTION=" << config.videoResolution << "\n"
      << "VIDEO_FPS=" << config.videoFps << "\n"
      << "VIDEO_FORMAT=" << config.videoFormat << "\n"
      << "AUDIO_FORMAT=" << config.audioFormat << "\n"
      << "CODEC_RECOMPILER=" << (config.codecRecompiler ? "true" : "false") << "\n"
      << "USE_ARCHIVE=" << (config.useArchive ? "true" : "false") << "\n"
      << "ONLY_AUDIO=" << (config.onlyAudio ? "true" : "false") << "\n"
      << "ONLY_VIDEO=" << (config.onlyVideo ? "true" : "false") << "\n"
      << "PLAYLIST_DELAY_ENABLED=" << (config.playlistDelayEnabled ? "true" : "false") << "\n"
      << "PLAYLIST_DELAY_INTERVAL=" << config.playlistDelayInterval << "\n"
      << "PLAYLIST_DELAY_DURATION=" << config.playlistDelayDuration << "\n"
      << "LANGUAGE=" << (config.language == domain::Language::Russian ? "ru" : "en") << "\n";

    f.close();
    return true;
}

} // namespace mrcli::services
