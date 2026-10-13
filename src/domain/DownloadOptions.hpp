#pragma once
#include <string>
#include <optional>

namespace mrcli::domain {

struct DownloadOptions {
    std::string url;
    std::string playlistStart;
    std::string playlistEnd;
    bool isPlaylist{false};
    bool isRetry{false};
};

struct ProgressInfo {
    std::string percent;
    std::string speed;
    std::string eta;
    int currentItem{0};
    int totalItems{0};
    std::string currentVideoId;
    std::string mediaTitle;
    bool isAudio{false};
};

} // namespace mrcli::domain
