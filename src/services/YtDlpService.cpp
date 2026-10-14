#include "YtDlpService.hpp"
#include "FormatBuilder.hpp"
#include "OutputParser.hpp"
#include "src/core/Process.hpp"
#include "src/core/StringUtil.hpp"
#include "src/core/FileSystem.hpp"
#include <conio.h>
#include <iostream>
#include <filesystem>

namespace mrcli::services {

YtDlpService::YtDlpService(std::shared_ptr<PathProvider> paths,
                           std::shared_ptr<ArchiveService> archive,
                           std::shared_ptr<CookieService> cookies,
                           std::shared_ptr<ILogger> logger,
                           std::shared_ptr<LocalizationService> loc)
    : m_paths(std::move(paths)),
      m_archive(std::move(archive)),
      m_cookies(std::move(cookies)),
      m_logger(std::move(logger)),
      m_loc(std::move(loc)) {}

std::wstring YtDlpService::buildCommand(const domain::AppConfig& config,
                                        const domain::DownloadOptions& options,
                                        const domain::DependencyStatus& deps) const {
    std::string ytdlpExe = deps.ytDlpPath.empty() ? (m_paths->getConfigDir() + "yt-dlp.exe") : deps.ytDlpPath;
    std::wstring cmd = L"\"" + core::StringUtil::utf8ToWstring(ytdlpExe) + L"\"";

    if (config.useCookies && m_cookies->hasSavedCookies()) {
        cmd += L" --cookies \"" + core::StringUtil::utf8ToWstring(m_cookies->getCookiesPath()) + L"\"";
    }

    if (config.useArchive && !m_archive->getArchivePath().empty()) {
        cmd += L" --download-archive \"" + core::StringUtil::utf8ToWstring(m_archive->getArchivePath()) + L"\"";
    }

    std::string fmt = FormatBuilder::buildFormat(config);
    cmd += L" -f \"" + core::StringUtil::utf8ToWstring(fmt) + L"\"";

    cmd += L" --windows-filenames --no-mtime --concurrent-fragments 4 --retries 10 --fragment-retries 10";

    if (deps.ffmpegInstalled && !deps.ffmpegPath.empty()) {
        std::string ffmpegDir = core::FileSystem::getDirectoryName(deps.ffmpegPath);
        cmd += L" --ffmpeg-location \"" + core::StringUtil::utf8ToWstring(ffmpegDir) + L"\"";
    }

    if (options.isPlaylist) {
        cmd += L" --sleep-requests 1 --sleep-interval 2 --max-sleep-interval 5";
    }

    if (deps.quickJsInstalled && !deps.quickJsPath.empty() && core::FileSystem::fileExists(deps.quickJsPath)) {
        cmd += L" --js-runtimes \"quickjs:" + core::StringUtil::utf8ToWstring(deps.quickJsPath) + L"\"";
    }

    if (config.codecRecompiler) {
        cmd += L" --recode-video mp4";
    }

    std::string tag = "";
    if (config.onlyVideo) {
        tag = " [only video]";
        cmd += L" --no-audio";
        if (config.videoFormat.find("WEBM") != std::string::npos) cmd += L" --merge-output-format webm";
        else cmd += L" --merge-output-format mp4";
    }
    else if (config.onlyAudio) {
        tag = " [only audio]";
        cmd += L" -x";
        if (config.audioFormat.find("M4A") != std::string::npos) cmd += L" --audio-format m4a";
        else if (config.audioFormat.find("Opus") != std::string::npos) cmd += L" --audio-format opus";
        else if (config.audioFormat.find("Vorbis") != std::string::npos) cmd += L" --audio-format ogg";
        else cmd += L" --audio-format m4a";
    }
    else {
        if (config.videoFormat.find("WEBM") != std::string::npos) cmd += L" --merge-output-format webm";
        else cmd += L" --merge-output-format mp4";
    }

    if (options.isPlaylist) {
        if (!options.startIndex.empty()) cmd += L" --playlist-start " + core::StringUtil::utf8ToWstring(options.startIndex);
        if (!options.endIndex.empty()) cmd += L" --playlist-end " + core::StringUtil::utf8ToWstring(options.endIndex);
    }

    cmd += L" --encoding utf-8";

    std::string out = options.isPlaylist ?
        config.downloadPath + "[Playlist] %(playlist_title)s/%(playlist_index)03d - [%(channel)s] %(title)s" + tag :
        config.downloadPath + "%(title)s" + tag;

    out += ".%(ext)s";
    cmd += L" -o \"" + core::StringUtil::utf8ToWstring(out) + L"\"";
    cmd += L" \"" + core::StringUtil::utf8ToWstring(options.url) + L"\"";

    return cmd;
}

bool YtDlpService::manualFfmpegMerge(const std::string& mergeErrorPath, const std::string& ffmpegPath) {
    if (mergeErrorPath.empty() || ffmpegPath.empty() || !core::FileSystem::fileExists(ffmpegPath)) {
        return false;
    }

    m_logger->warn(m_loc->get(StrId::MergeFailedManual));

    std::string dir = core::FileSystem::getDirectoryName(mergeErrorPath);
    std::string baseName = core::FileSystem::getFileName(mergeErrorPath);
    size_t dot = baseName.find_last_of('.');
    if (dot != std::string::npos) baseName = baseName.substr(0, dot);

    std::string videoFile = "", audioFile = "";
    auto files = core::FileSystem::listFiles(dir);
    for (const auto& file : files) {
        std::string fileName = core::FileSystem::getFileName(file);
        if (fileName.find(baseName) != std::string::npos) {
            if (fileName.find(".mp4") != std::string::npos && fileName.find(".m4a") == std::string::npos) {
                videoFile = file;
            }
            else if (fileName.find(".m4a") != std::string::npos || fileName.find(".webm") != std::string::npos) {
                audioFile = file;
            }
        }
    }

    if (!videoFile.empty() && !audioFile.empty()) {
        std::string outputFile = dir + "\\" + baseName + "_merged.mp4";
        std::string mergeCmd = "\"" + ffmpegPath + "\" -i \"" + videoFile + "\" -i \"" + audioFile + "\" -c:v copy -c:a aac -y \"" + outputFile + "\"";

        m_logger->info(m_loc->get(StrId::ManualMergeRun) + mergeCmd);
        int exitCode = core::Process::executeWait(core::StringUtil::utf8ToWstring(mergeCmd));

        if (exitCode == 0) {
            m_logger->info(m_loc->get(StrId::ManualMergeSuccess));
            core::FileSystem::deleteFile(videoFile);
            core::FileSystem::deleteFile(audioFile);
            std::string finalFile = dir + "\\" + baseName + ".mp4";
            if (core::FileSystem::renameFile(outputFile, finalFile)) {
                m_logger->info(m_loc->get(StrId::FinalFile) + finalFile);
            } else {
                m_logger->warn(m_loc->get(StrId::MergedFileCreatedAs) + outputFile);
            }
            return true;
        }
    }
    return false;
}

domain::DownloadResult YtDlpService::execute(const domain::AppConfig& config,
                                            const domain::DownloadOptions& options,
                                            const domain::DependencyStatus& deps,
                                            IDownloadObserver* observer) {
    domain::DownloadResult result;
    result.error = domain::ErrorType::None;

    std::wstring cmd = buildCommand(config, options, deps);

    core::Process process;
    if (!process.spawnWithPipes(cmd)) {
        m_logger->error(m_loc->tr("Failed to launch yt-dlp process! Error code: ", "Не удалось запустить процесс yt-dlp! Код ошибки: ") + std::to_string(GetLastError()));
        result.error = domain::ErrorType::GenericVideoError;
        return result;
    }

    bool isPlaylist = options.isPlaylist;
    bool mergeFailed = false;
    int curItem = 0, totalItems = 0;
    int completedVideosCount = 0;
    int lastDelayAtCount = 0;
    std::string currentVideoId = "";
    bool currentVideoCompleted = false;
    std::string mergeErrorPath = "";
    bool sawPotWarning = false;

    std::string line;
    char buffer[4096];

    auto checkAndHandleEscape = [&]() -> bool {
        HWND fg = GetForegroundWindow();
        HWND console = GetConsoleWindow();
        bool consoleFocused = (fg == nullptr || fg == console);
        bool escHit = false;

        if (consoleFocused && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
            escHit = true;
        } else if (_kbhit()) {
            int key = _getch();
            if (key == 27) {
                escHit = true;
            }
        }

        if (escHit) {
            process.suspend();
            m_logger->warn(m_loc->get(StrId::StoppedByUser));

            // Flush buffered input so repeated ESC keypresses don't accidentally close the pause menu
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
            while (_kbhit()) { (void)_getch(); }
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { Sleep(10); }
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
            while (_kbhit()) { (void)_getch(); }

            int action = observer ? observer->onPauseRequested(isPlaylist || totalItems > 1) : 0;

            if (action == 0) {
                m_logger->success(m_loc->get(StrId::ResumingDownload));
                process.resume();
                return false;
            }
            else if (action == 1) {
                process.terminate(1);
                if (!currentVideoId.empty()) {
                    m_archive->removeId(currentVideoId);
                }
                if (isPlaylist || totalItems > 1) {
                    result.error = domain::ErrorType::SkipCurrentVideo;
                    m_logger->warn(m_loc->get(StrId::CurrentVideoCancelled));
                } else {
                    result.error = domain::ErrorType::CancelledByUser;
                    m_logger->warn(m_loc->get(StrId::VideoCancelledByUser));
                }
                return true;
            }
            else {
                process.terminate(1);
                if (!currentVideoId.empty()) {
                    m_archive->removeId(currentVideoId);
                }
                result.error = domain::ErrorType::CancelledByUser;
                m_logger->warn(m_loc->get(StrId::AllVideosCancelledByUser));
                return true;
            }
        }
        return false;
    };

    while (true) {
        if (checkAndHandleEscape()) {
            break;
        }

        size_t bytesRead = 0;
        if (!process.readOutput(buffer, sizeof(buffer) - 1, bytesRead)) {
            break;
        }

        if (bytesRead == 0) {
            if (!process.isRunning()) {
                break;
            }
            if (checkAndHandleEscape()) {
                break;
            }
            Sleep(15);
            continue;
        }

        for (size_t i = 0; i < bytesRead; ++i) {
            char c = buffer[i];
            if (c == '\r' || c == '\n') {
                if (!line.empty()) {
                    auto parseRes = OutputParser::parseLine(line, config.onlyAudio, config.onlyVideo);

                    if (!parseRes.possibleVideoId.empty() && parseRes.possibleVideoId != currentVideoId) {
                        if (!currentVideoCompleted && !currentVideoId.empty()) {
                            m_archive->removeId(currentVideoId);
                        }
                        currentVideoId = parseRes.possibleVideoId;
                        currentVideoCompleted = false;
                    }

                    if (parseRes.event == LineEvent::ItemTransition) {
                        if (curItem > 0 && parseRes.curItem != curItem) {
                            if (observer) {
                                observer->onItemTransition(parseRes.curItem, parseRes.totalItems);
                            }
                            if (config.playlistDelayEnabled && config.playlistDelayInterval > 0 && config.playlistDelayDuration > 0) {
                                if (completedVideosCount > 0 && completedVideosCount % config.playlistDelayInterval == 0 && completedVideosCount != lastDelayAtCount) {
                                    lastDelayAtCount = completedVideosCount;
                                    if (observer && !observer->onPlaylistDelay(completedVideosCount, config.playlistDelayDuration)) {
                                        result.error = domain::ErrorType::CancelledByUser;
                                        if (!currentVideoId.empty()) {
                                            m_archive->removeId(currentVideoId);
                                        }
                                        process.terminate(1);
                                        break;
                                    }
                                }
                            }
                        }
                        if (parseRes.curItem != curItem) {
                            currentVideoCompleted = false;
                        }
                        curItem = parseRes.curItem;
                        totalItems = parseRes.totalItems;
                        result.totalItems = totalItems;
                        result.lastProcessedItem = curItem;
                        isPlaylist = (totalItems > 0);
                    }
                    else if (parseRes.event == LineEvent::AlreadyInArchive) {
                        if (!currentVideoCompleted) {
                            currentVideoCompleted = true;
                            completedVideosCount++;
                        }
                        std::string archivedName = parseRes.mediaName;

                        bool isJustId = archivedName.empty() || (archivedName.find(' ') == std::string::npos && archivedName.size() <= 20);
                        if (isJustId) {
                            std::string foundOnDisk = "";
                            std::error_code ec;
                            if (std::filesystem::exists(config.downloadPath, ec)) {
                                char idxBuf3[16] = {0};
                                char idxBuf2[16] = {0};
                                if (curItem > 0) {
                                    snprintf(idxBuf3, sizeof(idxBuf3), "%03d - ", curItem);
                                    snprintf(idxBuf2, sizeof(idxBuf2), "%02d - ", curItem);
                                }

                                for (const auto& entry : std::filesystem::recursive_directory_iterator(config.downloadPath, std::filesystem::directory_options::skip_permission_denied, ec)) {
                                    if (entry.is_regular_file(ec)) {
                                        std::string stem = entry.path().stem().u8string();
                                        if ((idxBuf3[0] && stem.rfind(idxBuf3, 0) == 0) ||
                                            (idxBuf2[0] && stem.rfind(idxBuf2, 0) == 0) ||
                                            (!archivedName.empty() && stem.find(archivedName) != std::string::npos)) {
                                            foundOnDisk = stem;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!foundOnDisk.empty()) {
                                archivedName = foundOnDisk;
                            } else if (curItem > 0) {
                                char buf[16];
                                snprintf(buf, sizeof(buf), "%03d", curItem);
                                archivedName = buf;
                            }
                        }

                        if (observer) {
                            observer->onVideoArchived(curItem, totalItems, archivedName);
                        }
                    }
                    else if (parseRes.event == LineEvent::Destination) {
                        if (!parseRes.destinationPath.empty()) {
                            std::string dir = core::FileSystem::getDirectoryName(parseRes.destinationPath);
                            if (!dir.empty()) result.savedDirectory = dir;
                        }
                        if (observer) {
                            observer->onMediaItemStarted(curItem, totalItems, parseRes.mediaName, parseRes.isAudio);
                        }
                    }
                    else if (parseRes.event == LineEvent::Merger) {
                        if (!currentVideoCompleted) {
                            currentVideoCompleted = true;
                            completedVideosCount++;
                        }
                        if (!parseRes.mediaName.empty()) {
                            std::string dir = core::FileSystem::getDirectoryName(parseRes.mediaName);
                            if (!dir.empty()) result.savedDirectory = dir;
                        }
                        if (observer) {
                            observer->onMergeStarted(parseRes.mediaName);
                        }
                    }
                    else if (parseRes.event == LineEvent::Progress) {
                        if (parseRes.progress.percent >= 99.9 && (config.onlyAudio || config.onlyVideo)) {
                            if (!currentVideoCompleted) {
                                currentVideoCompleted = true;
                                completedVideosCount++;
                            }
                        }
                        if (observer) {
                            observer->onProgress(parseRes.progress);
                        }
                        if (checkAndHandleEscape()) break;
                    }
                    else if (parseRes.event == LineEvent::Warning || parseRes.event == LineEvent::Error) {
                        if (parseRes.error != domain::ErrorType::None) {
                            result.error = parseRes.error;
                            if (parseRes.error == domain::ErrorType::PoTokenBotguard) {
                                sawPotWarning = true;
                            }
                        }
                        if (line.find("merge") != std::string::npos || line.find("Merger") != std::string::npos) {
                            mergeFailed = true;
                            size_t q1 = line.find('"');
                            if (q1 != std::string::npos) {
                                size_t q2 = line.find('"', q1 + 1);
                                if (q2 != std::string::npos) {
                                    mergeErrorPath = line.substr(q1 + 1, q2 - q1 - 1);
                                }
                            }
                        }
                        if (observer) {
                            observer->onMessage(line, parseRes.event == LineEvent::Error ? domain::LogLevel::Error : domain::LogLevel::Warning);
                        }
                    }

                    line.clear();
                }
            } else {
                line += c;
            }
        }
        if (result.error == domain::ErrorType::CancelledByUser) break;
    }

    int exitCode = process.waitForExit();
    result.exitCode = exitCode;

    if (result.error == domain::ErrorType::CancelledByUser || result.error == domain::ErrorType::SkipCurrentVideo) {
        if (!currentVideoId.empty()) {
            m_archive->removeId(currentVideoId);
        }
        result.success = false;
        return result;
    }

    if (mergeFailed && !mergeErrorPath.empty() && deps.ffmpegInstalled) {
        if (manualFfmpegMerge(mergeErrorPath, deps.ffmpegPath)) {
            result.success = true;
            result.error = domain::ErrorType::None;
            return result;
        }
    }

    if (mergeFailed && !currentVideoId.empty()) {
        m_archive->removeId(currentVideoId);
    }

    if (!currentVideoCompleted && !currentVideoId.empty() && (result.error != domain::ErrorType::None || exitCode != 0)) {
        m_archive->removeId(currentVideoId);
    }

    if (exitCode != 0 && result.error == domain::ErrorType::None) {
        result.error = sawPotWarning ? domain::ErrorType::PoTokenBotguard : domain::ErrorType::GenericVideoError;
    }

    if (exitCode == 0 && !mergeFailed) {
        result.error = domain::ErrorType::None;
        result.success = true;
    } else {
        result.success = false;
    }

    if (result.savedDirectory.empty()) {
        result.savedDirectory = config.downloadPath;
        if (options.isPlaylist) {
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(config.downloadPath, ec)) {
                if (entry.is_directory(ec) && entry.path().filename().u8string().rfind("[Playlist] ", 0) == 0) {
                    result.savedDirectory = entry.path().u8string();
                    break;
                }
            }
        }
    }

    return result;
}

} // namespace mrcli::services
