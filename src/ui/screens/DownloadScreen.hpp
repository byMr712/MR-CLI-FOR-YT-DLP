#pragma once

#include "IScreen.hpp"
#include "src/app/AppContext.hpp"
#include "src/services/IDownloadObserver.hpp"

namespace mrcli::ui::screens {

class DownloadScreen : public IScreen, public services::IDownloadObserver {
public:
    explicit DownloadScreen(app::AppContext& ctx);
    void show() override;

    void start(const std::string& url = "",
               const std::string& startIdx = "",
               const std::string& endIdx = "",
               bool isPlaylist = false,
               bool retry = false);

    // IDownloadObserver implementation
    void onProgress(const domain::ProgressInfo& info) override;
    void onItemTransition(int current, int total) override;
    void onMediaItemStarted(int current, int total, const std::string& name, bool isAudio) override;
    void onVideoArchived(int current, int total, const std::string& name) override;
    void onMergeStarted(const std::string& outputPath) override;
    void onMessage(const std::string& message, domain::LogLevel level) override;
    int onPauseRequested(bool isMultiVideo) override;
    bool onPlaylistDelay(int completedCount, int durationSeconds) override;

private:
    app::AppContext& m_ctx;
    bool m_progressActive = false;
    bool m_hasOutputSinceLastTransition = false;
    int m_lastMediaItemIndex = 0;
    std::vector<std::string> m_skippedVideos;
};

} // namespace mrcli::ui::screens
