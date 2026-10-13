#include "FormatBuilder.hpp"

namespace mrcli::services {

std::string FormatBuilder::buildVideoFilter(const domain::AppConfig& config) {
    std::string videoFilter = "bestvideo[height<=" + config.videoResolution + "][fps<=" + config.videoFps + "]";

    if (config.videoFormat.find("AV1") != std::string::npos) {
        videoFilter += "[vcodec^=av01]";
    }
    else if (config.videoFormat.find("H.264") != std::string::npos) {
        if (config.videoResolution == "2160" || config.videoResolution == "1440") {
            videoFilter += "[vcodec^=vp9]";
        } else {
            videoFilter += "[vcodec^=avc1]";
        }
    }
    else if (config.videoFormat.find("VP9") != std::string::npos) {
        videoFilter += "[vcodec^=vp9]";
    }

    if (config.videoFormat.find("MP4") != std::string::npos) {
        if (config.videoResolution == "2160" || config.videoResolution == "1440") {
            videoFilter += "[ext=webm]";
        } else {
            videoFilter += "[ext=mp4]";
        }
    }
    else if (config.videoFormat.find("WEBM") != std::string::npos) {
        videoFilter += "[ext=webm]";
    }

    return videoFilter;
}

std::string FormatBuilder::buildAudioFilter(const domain::AppConfig& config) {
    std::string audioFilter = "bestaudio";
    if (config.audioFormat.find("AAC") != std::string::npos) {
        audioFilter += "[acodec^=mp4a][ext=m4a]";
    }
    else if (config.audioFormat.find("Opus") != std::string::npos) {
        audioFilter += "[acodec^=opus][ext=webm]";
    }
    else if (config.audioFormat.find("Vorbis") != std::string::npos) {
        audioFilter += "[acodec^=vorbis][ext=webm]";
    }
    return audioFilter;
}

std::string FormatBuilder::buildFormat(const domain::AppConfig& config) {
    if (config.onlyAudio) {
        return buildAudioFilter(config) + "/bestaudio/best";
    }

    std::string videoFilter = buildVideoFilter(config);
    std::string audioFilter = buildAudioFilter(config);

    if (config.onlyVideo) {
        return videoFilter + "/bestvideo[height<=" + config.videoResolution + "]/bestvideo/best";
    }

    std::string fallbackChain =
        videoFilter + "+" + audioFilter + "/" +
        videoFilter + "+bestaudio/" +
        "bestvideo[height<=" + config.videoResolution + "]+bestaudio/" +
        "bestvideo[height<=" + config.videoResolution + "]+best/" +
        "bestvideo+bestaudio/" +
        "best";

    return fallbackChain;
}

} // namespace mrcli::services
