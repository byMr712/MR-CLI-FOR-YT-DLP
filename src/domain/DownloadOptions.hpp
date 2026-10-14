#pragma once
#include <string>
#include <optional>
#include "Types.hpp"

namespace mrcli::domain {

struct DownloadOptions {
    std::string url;
    std::string playlistStart;
    std::string playlistEnd;
    std::string startIndex;
    std::string endIndex;
    bool isPlaylist{false};
    bool isRetry{false};
};

struct ProgressInfo {
    double percent{0.0};
    std::string percentStr;
    std::string speed;
    std::string eta;
    int currentItem{0};
    int totalItems{0};
    std::string currentVideoId;
    std::string mediaTitle;
    bool isAudio{false};
};

struct DownloadResult {
    bool success{false};
    ErrorType error{ErrorType::None};
    int exitCode{0};
    int lastProcessedItem{0};
    int totalItems{0};
    std::string savedDirectory;
};

} // namespace mrcli::domain
