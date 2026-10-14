#include "OutputParser.hpp"
#include "src/core/StringUtil.hpp"
#include <sstream>
#include <cctype>

namespace mrcli::services {

bool OutputParser::isAudioStream(std::string_view path, std::string_view ext, bool onlyAudio, bool onlyVideo) noexcept {
    if (onlyAudio) return true;
    if (onlyVideo) return false;

    std::string extLower = std::string(ext);
    for (char &ch : extLower) ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));

    if (extLower == "m4a" || extLower == "opus" || extLower == "ogg" || extLower == "mp3" ||
        extLower == "flac" || extLower == "wav" || extLower == "aac" || extLower == "weba" ||
        extLower == "oga" || extLower == "mka") {
        return true;
    }

    static const char* audioTags[] = {
        ".f139.", ".f140.", ".f141.", ".f249.", ".f250.", ".f251.", ".f599.", ".f600.",
        ".ba.", ".audio.", "_audio."
    };
    for (const char* tag : audioTags) {
        if (path.find(tag) != std::string_view::npos) return true;
    }

    if (extLower == "mp4" || extLower == "mkv" || extLower == "avi" ||
        extLower == "flv" || extLower == "mov" || extLower == "wmv" ||
        extLower == "ts" || extLower == "m4v") {
        return false;
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

ParsedLineResult OutputParser::parseLine(std::string_view line, bool onlyAudio, bool onlyVideo) {
    ParsedLineResult result;
    result.text = std::string(line);

    if (line.empty() || line == "ERROR:" || line == "ERROR: ") {
        result.event = LineEvent::None;
        return result;
    }

    // Extract Video ID from extractor lines (e.g. "[youtube] aqK0FqOQtvY: ...")
    if (line[0] == '[') {
        size_t bClose = line.find(']');
        if (bClose != std::string_view::npos && bClose + 2 < line.size()) {
            size_t colon = line.find(':', bClose + 1);
            if (colon != std::string_view::npos && colon > bClose + 2) {
                std::string possibleId = std::string(line.substr(bClose + 2, colon - (bClose + 2)));
                possibleId = core::StringUtil::trim(possibleId);
                if (possibleId != "Destination" && possibleId.find(' ') == std::string::npos &&
                    possibleId.size() >= 4 && possibleId.size() <= 64) {
                    result.possibleVideoId = possibleId;
                }
            }
        }
    }

    if (line.find("[download] Downloading item ") == 0) {
        int cur = 0, total = 0;
        if (sscanf_s(line.data(), "[download] Downloading item %d of %d", &cur, &total) == 2) {
            result.event = LineEvent::ItemTransition;
            result.curItem = cur;
            result.totalItems = total;
            result.progress.currentItem = cur;
            result.progress.totalItems = total;
            return result;
        }
    }

    if (line.find("has already been recorded in the archive") != std::string_view::npos ||
        line.find("has already been downloaded") != std::string_view::npos) {
        result.event = LineEvent::AlreadyInArchive;

        if (line.find("has already been downloaded") != std::string_view::npos) {
            size_t downPos = line.find("has already been downloaded");
            std::string path = std::string(line.substr(0, downPos));
            if (path.rfind("[download] ", 0) == 0) {
                path = path.substr(11);
            }
            path = core::StringUtil::trim(path);
            while (!path.empty() && (path.front() == '"' || path.front() == '\'')) path.erase(path.begin());
            while (!path.empty() && (path.back() == '"' || path.back() == '\'')) path.pop_back();
            result.destinationPath = path;

            size_t slash = path.find_last_of("\\/");
            std::string name = (slash != std::string::npos) ? path.substr(slash + 1) : path;
            size_t dot = name.find_last_of('.');
            if (dot != std::string::npos) {
                name = name.substr(0, dot);
            }

            size_t fDot = name.find_last_of('.');
            if (fDot != std::string::npos && fDot + 1 < name.size() && (name[fDot + 1] == 'f' || name[fDot + 1] == 'F')) {
                bool allDigits = true;
                for (size_t k = fDot + 2; k < name.size(); ++k) {
                    if (!isdigit(static_cast<unsigned char>(name[k]))) { allDigits = false; break; }
                }
                if (allDigits && fDot + 2 <= name.size()) {
                    name = name.substr(0, fDot);
                }
            }

            result.mediaName = name;
            return result;
        }

        size_t archPos = line.find("has already been recorded in the archive");
        std::string raw = std::string(line.substr(0, archPos));
        if (raw.rfind("[download] ", 0) == 0) {
            raw = raw.substr(11);
        }
        raw = core::StringUtil::trim(raw);
        while (!raw.empty() && (raw.back() == ':' || raw.back() == ' ')) {
            raw.pop_back();
        }
        raw = core::StringUtil::trim(raw);

        size_t colonPos = raw.find(": ");
        if (colonPos != std::string::npos && colonPos + 2 < raw.size()) {
            std::string afterColon = core::StringUtil::trim(raw.substr(colonPos + 2));
            if (!afterColon.empty()) {
                raw = afterColon;
            }
        }

        size_t slash = raw.find_last_of("\\/");
        if (slash != std::string::npos) {
            std::string name = raw.substr(slash + 1);
            size_t dot = name.find_last_of('.');
            if (dot != std::string::npos) {
                name = name.substr(0, dot);
            }
            raw = name;
        }

        result.mediaName = raw;
        result.destinationPath = raw;
        return result;
    }

    if (line.find("[download] Destination:") == 0) {
        result.event = LineEvent::Destination;
        std::string path = std::string(line.substr(23));
        path = core::StringUtil::trim(path);
        result.destinationPath = path;

        size_t slash = path.find_last_of("\\/");
        std::string name = (slash != std::string::npos) ? path.substr(slash + 1) : path;
        size_t dot = name.find_last_of('.');
        std::string ext = "";
        if (dot != std::string::npos) {
            ext = name.substr(dot + 1);
            name = name.substr(0, dot);
        }

        size_t fDot = name.find_last_of('.');
        if (fDot != std::string::npos && fDot + 1 < name.size() && (name[fDot + 1] == 'f' || name[fDot + 1] == 'F')) {
            bool allDigits = true;
            for (size_t k = fDot + 2; k < name.size(); ++k) {
                if (!isdigit(static_cast<unsigned char>(name[k]))) { allDigits = false; break; }
            }
            if (allDigits && fDot + 2 <= name.size()) {
                name = name.substr(0, fDot);
            }
        }

        result.mediaName = name;
        result.isAudio = isAudioStream(path, ext, onlyAudio, onlyVideo);
        result.progress.mediaTitle = name;
        result.progress.isAudio = result.isAudio;
        return result;
    }

    if (line.find("[Merger] Merging formats into") == 0) {
        result.event = LineEvent::Merger;
        std::string path = std::string(line.substr(29));
        path = core::StringUtil::trim(path);
        while (!path.empty() && (path.front() == '"' || path.front() == '\'')) path.erase(path.begin());
        while (!path.empty() && (path.back() == '"' || path.back() == '\'')) path.pop_back();
        result.destinationPath = path;
        result.mediaName = path;
        return result;
    }

    std::string pct, spd, eta;
    if (parseDownloadProgress(line, pct, spd, eta)) {
        result.event = LineEvent::Progress;
        result.progress.percentStr = pct;
        try {
            result.progress.percent = std::stod(pct);
        } catch (...) {
            result.progress.percent = 0.0;
        }
        result.progress.speed = spd;
        result.progress.eta = eta;
        return result;
    }

    domain::ErrorType err = classifyError(line);
    if (err != domain::ErrorType::None) {
        result.event = (line.find("ERROR") != std::string_view::npos) ? LineEvent::Error : LineEvent::Warning;
        result.error = err;
        result.errorType = err;
        return result;
    }

    if (line.find("ERROR") != std::string_view::npos) {
        result.event = LineEvent::Error;
        result.error = domain::ErrorType::Video;
        result.errorType = domain::ErrorType::Video;

        if (line.find("merge") != std::string_view::npos || line.find("Merger") != std::string_view::npos) {
            result.event = LineEvent::MergeFailed;
            result.error = domain::ErrorType::Merge;
            result.errorType = domain::ErrorType::Merge;

            size_t q1 = line.find('"');
            if (q1 != std::string_view::npos) {
                size_t q2 = line.find('"', q1 + 1);
                if (q2 != std::string_view::npos) {
                    result.mergeErrorPath = std::string(line.substr(q1 + 1, q2 - q1 - 1));
                }
            }
        }
        return result;
    }

    if (line.find("WARNING") != std::string_view::npos) {
        result.event = LineEvent::Warning;
        return result;
    }

    return result;
}

} // namespace mrcli::services
