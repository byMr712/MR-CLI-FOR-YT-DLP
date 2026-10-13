#pragma once
#include <string>
#include <string_view>

namespace mrcli::domain {

enum class Language {
    English = 0,
    Russian = 1
};

enum class ErrorType {
    None = 0,
    Cookie = 1,
    Age = 2,
    Live = 3,
    Merge = 4,
    Video = 5,
    RateLimit = 6,
    CancelledByUser = 7,
    PoToken = 8,
    Unavailable = 9,
    Drm = 10,
    Format = 11,
    Network = 12,
    SkipCurrentVideo = 13
};

enum class MediaKind {
    Video,
    Audio
};

enum class DownloadState {
    Idle,
    Downloading,
    Paused,
    Merging,
    Completed,
    Failed,
    Cancelled
};

} // namespace mrcli::domain
