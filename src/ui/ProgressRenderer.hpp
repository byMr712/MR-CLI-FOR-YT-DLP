#pragma once

#include <string>
#include <functional>
#include "src/domain/DownloadOptions.hpp"

namespace mrcli::ui {

class ProgressRenderer {
public:
    static void renderProgressBar(const domain::ProgressInfo& info, const std::string& etaLabel = "ETA ");
    static void renderComponentProgress(const std::string& label, double percent, const std::string& extraInfo = "");
    static bool renderDelayCountdown(int completedCount, int durationSeconds,
                                    const std::function<std::string(const std::string&, const std::string&)>& tr);
};

} // namespace mrcli::ui
