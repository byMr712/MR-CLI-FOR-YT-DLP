#include "OutputParser.hpp"
#include "../core/StringUtil.hpp"
#include <sstream>
#include <cctype>

namespace mrcli::services {

bool OutputParser::isAudioStream(std::string_view path, std::string_view ext) const noexcept {
    if (ext == "m4a" || ext == "opus" || ext == "ogg" || ext == "mp3" || ext == "flac" || ext == "wav" || ext == "aac") {
        return true;
    }

    static const char* audioTags[] = {
        ".f139.", ".f140.", ".f141.", ".f249.", ".f250.", ".f251.", ".f599.", ".f600.",
        ".ba.", ".audio.", "_audio.", ".m4a.", ".opus.", ".ogg.", ".aac."
    };
    for (const char* tag : audioTags) {
        if (path.find(tag) != std::string_view::npos) return true;
    }

    static const char* videoTags[] = {
        ".f133.", ".f134.", ".f135.", ".f136.", ".f137.", ".f138.",
        ".f160.", ".f242.", ".f243.", ".f244.", ".f247.", ".f248.",
        ".f271.", ".f272.", ".f278.", ".bv.", ".video.", "_video."
    };
    for (const char* tag : videoTags) {
        if (path.find(tag) != std::string_view::npos) return false;
    }

    return false;
}

bool OutputParser::parseDownloadProgress(
    std::string_view line,
    std::string& percent,
    std::string& speed,
    std::string& eta
) {
    if (line.find("[download]") == std::string_view::npos) return false;

    size_t pctPos = line.find('%');
    if (pctPos == std::string_view::npos) return false;

    size_t numStart = line.rfind(' ', pctPos - 1);
    if (numStart == std::string_view::npos) numStart = line.find_first_of("0123456789");
    else numStart++;

    if (numStart < pctPos) {
        percent = std::string(line.substr(numStart, pctPos - numStart));
    } else {
        return false;
    }

    size_t atPos = line.find(" at ");
    if (atPos != std::string_view::npos) {
        size_t spdStart = atPos + 4;
        while (spdStart < line.size() && line[spdStart] == ' ') spdStart++;
        size_t spdEnd = line.find(' ', spdStart);
        if (spdEnd != std::string_view::npos) {
            speed = std::string(line.substr(spdStart, spdEnd - spdStart));
        }
    }

    size_t etaPos = line.find("ETA ");
    if (etaPos != std::string_view::npos) {
        size_t etaStart = etaPos + 4;
        while (etaStart < line.size() && line[etaStart] == ' ') etaStart++;
        size_t etaEnd = line.find(' ', etaStart);
        if (etaEnd != std::string_view::npos) {
            eta = std::string(line.substr(etaStart, etaEnd - etaStart));
        } else {
            eta = std::string(line.substr(etaStart));
        }
    }

    return !percent.empty();
}

domain::ErrorType OutputParser::classifyError(std::string_view line) {
    if (line.find("rate-limited by YouTube") != std::string_view::npos ||
        line.find("Your account has been rate-limited") != std::string_view::npos ||
        line.find("HTTP Error 429") != std::string_view::npos) {
        return domain::ErrorType::RateLimit;
    }

    if (line.find("require a GVS PO Token") != std::string_view::npos ||
        line.find("require a PO Token") != std::string_view::npos ||
        line.find("PO Token which was not provided") != std::string_view::npos ||
        line.find("PO-Token-Guide") != std::string_view::npos ||
        line.find("po_token=") != std::string_view::npos ||
        line.find("Sign in to confirm you're not a bot") != std::string_view::npos ||
        line.find("HTTP Error 403") != std::string_view::npos) {
        m_sawPotWarning = true;
        return domain::ErrorType::PoToken;
    }

    if (line.find("Sign in to confirm your age") != std::string_view::npos ||
        line.find("age-restricted") != std::string_view::npos) {
        return domain::ErrorType::Age;
    }

    if (line.find("Login required") != std::string_view::npos ||
        line.find("invalid cookies") != std::string_view::npos ||
        line.find("no longer valid") != std::string_view::npos ||
        line.find("cookies are expired") != std::string_view::npos) {
        return domain::ErrorType::Cookie;
    }

    if (line.find("This video is DRM protected") != std::string_view::npos ||
        line.find("DRM protected") != std::string_view::npos ||
        line.find("protected by DRM") != std::string_view::npos) {
        return domain::ErrorType::Drm;
    }

    if (line.find("Video unavailable") != std::string_view::npos ||
        line.find("This video is private") != std::string_view::npos ||
        line.find("Private video") != std::string_view::npos ||
        line.find("unavailable video is hidden") != std::string_view::npos ||
        line.find("Join this channel to get access") != std::string_view::npos ||
        line.find("Members-only") != std::string_view::npos ||
        line.find("not available in your country") != std::string_view::npos ||
        line.find("The uploader has not made this video available") != std::string_view::npos) {
        return domain::ErrorType::Unavailable;
    }

    if (line.find("This live event will begin in a few moments") != std::string_view::npos ||
        (line.find("is a live stream") != std::string_view::npos && line.find("has not started") != std::string_view::npos)) {
        return domain::ErrorType::Live;
    }

    if (line.find("Requested format is not available") != std::string_view::npos ||
        line.find("No video formats found") != std::string_view::npos) {
        return domain::ErrorType::Format;
    }

    if (line.find("Unable to download webpage") != std::string_view::npos ||
        line.find("Connection reset") != std::string_view::npos ||
        line.find("Network is unreachable") != std::string_view::npos ||
        line.find("IncompleteRead") != std::string_view::npos ||
        line.find("Remote end closed connection") != std::string_view::npos ||
        line.find("getaddrinfo failed") != std::string_view::npos) {
        return domain::ErrorType::Network;
    }

    return domain::ErrorType::None;
}

ParsedLineResult OutputParser::parseLine(std::string_view line) {
    ParsedLineResult result;
    result.text = std::string(line);

    if (line == "ERROR:" || line == "ERROR: ") {
        result.event = LineEvent::None;
        return result;
    }

    if (line.find("[download] Downloading item ") == 0) {
        int cur = 0, total = 0;
        if (sscanf_s(line.data(), "[download] Downloading item %d of %d", &cur, &total) == 2) {
            result.event = LineEvent::ItemTransition;
            result.progress.currentItem = cur;
            result.progress.totalItems = total;
            m_currentItem = cur;
            m_totalItems = total;
            return result;
        }
    }

    if (line.find("has already been recorded in the archive") != std::string_view::npos) {
        result.event = LineEvent::AlreadyInArchive;
        return result;
    }

    if (line.find("[download] Destination:") == 0) {
        result.event = LineEvent::Destination;
        std::string path = std::string(line.substr(23));
        result.destinationPath = core::StringUtil::trim(path);

        size_t slash = result.destinationPath.find_last_of("\\/");
        std::string name = (slash != std::string::npos) ? result.destinationPath.substr(slash + 1) : result.destinationPath;
        size_t dot = name.find_last_of('.');
        std::string ext = (dot != std::string::npos) ? name.substr(dot + 1) : "";
        result.progress.isAudio = isAudioStream(result.destinationPath, ext);
        result.progress.mediaTitle = name;
        return result;
    }

    if (line.find("[Merger] Merging formats into") == 0) {
        result.event = LineEvent::MergeStarted;
        std::string path = std::string(line.substr(29));
        result.destinationPath = core::StringUtil::trim(path);
        return result;
    }

    std::string pct, spd, eta;
    if (parseDownloadProgress(line, pct, spd, eta)) {
        result.event = LineEvent::ProgressUpdate;
        result.progress.percent = pct;
        result.progress.speed = spd;
        result.progress.eta = eta;
        result.progress.currentItem = m_currentItem;
        result.progress.totalItems = m_totalItems;
        return result;
    }

    domain::ErrorType err = classifyError(line);
    if (err != domain::ErrorType::None) {
        m_lastError = err;
        result.event = LineEvent::ErrorDetected;
        result.errorType = err;
        return result;
    }

    if (line.find("ERROR") != std::string_view::npos) {
        if (line.find("merge") != std::string_view::npos || line.find("Merger") != std::string_view::npos) {
            result.event = LineEvent::MergeFailed;
            result.errorType = domain::ErrorType::Merge;
            m_lastError = domain::ErrorType::Merge;

            size_t q1 = line.find('"');
            if (q1 != std::string_view::npos) {
                size_t q2 = line.find('"', q1 + 1);
                if (q2 != std::string_view::npos) {
                    result.mergeErrorPath = std::string(line.substr(q1 + 1, q2 - q1 - 1));
                }
            }
            return result;
        }

        result.event = LineEvent::ErrorDetected;
        result.errorType = m_sawPotWarning ? domain::ErrorType::PoToken : domain::ErrorType::Video;
        m_lastError = result.errorType;
        return result;
    }

    if (line.find("WARNING") != std::string_view::npos) {
        result.event = LineEvent::Warning;
        return result;
    }

    return result;
}

} // namespace mrcli::services
