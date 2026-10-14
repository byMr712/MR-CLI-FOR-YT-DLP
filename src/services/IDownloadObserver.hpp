#pragma once

#include <string>
#include "src/domain/DownloadOptions.hpp"
#include "src/domain/Types.hpp"

namespace mrcli::services {

class IDownloadObserver {
public:
    virtual ~IDownloadObserver() = default;
    virtual void onProgress(const domain::ProgressInfo& info) = 0;
    virtual void onItemTransition(int current, int total) {}
    virtual void onMediaItemStarted(int current, int total, const std::string& name, bool isAudio) = 0;
    virtual void onVideoArchived(int current, int total, const std::string& name) {}
    virtual void onMergeStarted(const std::string& outputPath) = 0;
    virtual void onMessage(const std::string& message, domain::LogLevel level) = 0;
    virtual int onPauseRequested(bool isMultiVideo) = 0; // 0=Resume, 1=Cancel Current, 2=Cancel All
    virtual bool onPlaylistDelay(int completedCount, int durationSeconds) = 0; // false if cancelled
};

} // namespace mrcli::services
