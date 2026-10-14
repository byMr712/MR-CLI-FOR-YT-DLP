#pragma once
#include <string>
#include <string_view>

namespace mrcli::domain {

enum class Language {
    English = 0,
    Russian = 1
};

enum class LogLevel {
    Info,
    Warning,
    Error,
    Debug
};

enum class ErrorType {
    None = 0,
    Cookie = 1,
    AuthenticationRequired = 1,
    Age = 2,
    AgeRestricted = 2,
    Live = 3,
    LiveStreamNotStarted = 3,
    Merge = 4,
    Video = 5,
    GenericVideoError = 5,
    RateLimit = 6,
    CancelledByUser = 7,
    PoToken = 8,
    PoTokenBotguard = 8,
    Unavailable = 9,
    VideoUnavailable = 9,
    Drm = 10,
    DrmProtected = 10,
    Format = 11,
    FormatNotAvailable = 11,
    Network = 12,
    NetworkError = 12,
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
