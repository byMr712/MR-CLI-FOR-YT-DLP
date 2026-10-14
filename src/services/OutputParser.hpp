#pragma once
#include "src/domain/Types.hpp"
#include "src/domain/DownloadOptions.hpp"
#include <string>
#include <string_view>

namespace mrcli::services {

enum class LineEvent {
    None,
    ItemTransition,
    AlreadyInArchive,
    Destination,
    Progress,
    ProgressUpdate,
    Merger,
    MergeStarted,
    MergeFinished,
    MergeFailed,
    Warning,
    Error,
    ErrorDetected
};

struct ParsedLineResult {
    LineEvent event{LineEvent::None};
    domain::ErrorType error{domain::ErrorType::None};
    domain::ErrorType errorType{domain::ErrorType::None};
    std::string text;
    std::string destinationPath;
    std::string mediaName;
    std::string mergeErrorPath;
    std::string possibleVideoId;
    int curItem{0};
    int totalItems{0};
    bool isAudio{false};
    domain::ProgressInfo progress;
};

class OutputParser {
public:
    static ParsedLineResult parseLine(std::string_view line, bool onlyAudio = false, bool onlyVideo = false);
    static bool isAudioStream(std::string_view path, std::string_view ext, bool onlyAudio = false, bool onlyVideo = false) noexcept;
    static bool parseDownloadProgress(std::string_view line, std::string& percent, std::string& speed, std::string& eta);
    static domain::ErrorType classifyError(std::string_view line);
};

} // namespace mrcli::services
