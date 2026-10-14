#pragma once
#include <string>
#include <filesystem>

namespace mrcli::domain {

struct DependencyInfo {
    std::string name;
    std::string executableName;
    std::filesystem::path resolvedPath;
    std::string downloadUrl;
    bool isArchive{false};
    bool isRequired{true};
    bool isFound{false};
};

struct DependencyStatus {
    bool ytDlpInstalled{false};
    std::string ytDlpPath;
    bool ffmpegInstalled{false};
    std::string ffmpegPath;
    bool quickJsInstalled{false};
    std::string quickJsPath;

    [[nodiscard]] bool isComplete() const noexcept {
        return ytDlpInstalled && ffmpegInstalled && quickJsInstalled;
    }
};

} // namespace mrcli::domain
