#pragma once
#include "../domain/Types.hpp"
#include "../domain/DownloadOptions.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace mrcli::services {

enum class LineEvent {
    None,
    ItemTransition,
    AlreadyInArchive,
    Destination,
    ProgressUpdate,
    MergeStarted,
    MergeFinished,
    MergeFailed,
    Warning,
    ErrorDetected
};

struct ParsedLineResult {
    LineEvent event{LineEvent::None};
    domain::ErrorType errorType{domain::ErrorType::None};
    std::string text;
    std::string destinationPath;
    std::string mergeErrorPath;
    domain::ProgressInfo progress;
};

class OutputParser {
public:
    OutputParser() = default;

    ParsedLineResult parseLine(std::string_view line);

    [[nodiscard]] domain::ErrorType getDetectedError() const noexcept { return m_lastError; }
    [[nodiscard]] bool sawPoTokenWarning() const noexcept { return m_sawPotWarning; }
    [[nodiscard]] bool isAudioStream(std::string_view path, std::string_view ext) const noexcept;

    static bool parseDownloadProgress(
        std::string_view line,
        std::string& percent,
        std::string& speed,
        std::string& eta
    );

private:
    domain::ErrorType classifyError(std::string_view line);

    domain::ErrorType m_lastError{domain::ErrorType::None};
    bool m_sawPotWarning{false};
    int m_currentItem{0};
    int m_totalItems{0};
};

} // namespace mrcli::services
