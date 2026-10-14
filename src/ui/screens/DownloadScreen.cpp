#include "DownloadScreen.hpp"
#include "CookieEditorScreen.hpp"
#include "src/ui/Menu.hpp"
#include "src/ui/Dialogs.hpp"
#include "src/ui/InputReader.hpp"
#include "src/ui/ProgressRenderer.hpp"
#include "src/core/Console.hpp"
#include "src/core/FileSystem.hpp"
#include "src/core/Keyboard.hpp"
#include "src/core/StringUtil.hpp"
#include <iostream>

namespace mrcli::ui::screens {

DownloadScreen::DownloadScreen(app::AppContext& ctx) : m_ctx(ctx) {}

void DownloadScreen::show() {
    start();
}

void DownloadScreen::onProgress(const domain::ProgressInfo& info) {
    ProgressRenderer::renderProgressBar(info, m_ctx.loc->tr("ETA ", "Осталось "));
    m_progressActive = true;
}

void DownloadScreen::onItemTransition(int current, int total) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    if (m_hasOutputSinceLastTransition) {
        std::cout << "\n";
        m_hasOutputSinceLastTransition = false;
    }
    m_lastMediaItemIndex = current;
}

void DownloadScreen::onMediaItemStarted(int current, int total, const std::string& name, bool isAudio) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    if (total > 0 && current > 1 && m_lastMediaItemIndex > 0 && current != m_lastMediaItemIndex && m_hasOutputSinceLastTransition) {
        std::cout << "\n";
    }
    m_lastMediaItemIndex = current;
    m_hasOutputSinceLastTransition = true;

    std::string mediaType = isAudio ? m_ctx.loc->tr("Audio", "Аудио") : m_ctx.loc->tr("Video", "Видео");
    if (total > 0) {
        core::Console::print(m_ctx.loc->tr("Downloading ", "Скачивание ") + mediaType + " " +
                             std::to_string(current) + m_ctx.loc->tr(" of ", " из ") + std::to_string(total) + " - " + name, core::ConsoleColor::Cyan);
    } else {
        core::Console::print(m_ctx.loc->tr("Downloading ", "Скачивание ") + mediaType + m_ctx.loc->tr(" into \"", " в \"") + name + "\"", core::ConsoleColor::Cyan);
    }
}

void DownloadScreen::onVideoArchived(int current, int total, const std::string& name) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    m_lastMediaItemIndex = current;
    m_hasOutputSinceLastTransition = true;

    std::string displayName = name;
    if (displayName.empty() && current > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%03d", current);
        displayName = buf;
    }

    core::Console::print(m_ctx.loc->tr("Downloading ", "Скачивание ") + displayName, core::ConsoleColor::Cyan);
    core::Console::print(m_ctx.loc->tr("Skipped - already downloaded according to archive",
                                       "Пропущено - по информации из архива уже скачано"), core::ConsoleColor::Yellow);
}

void DownloadScreen::onMergeStarted(const std::string& outputPath) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    m_hasOutputSinceLastTransition = true;
    core::Console::print(m_ctx.loc->tr("[Merger] Merging video and audio into \"", "[Слияние] Объединение видео и аудио в \"") + outputPath + "\"", core::ConsoleColor::Cyan);
}

void DownloadScreen::onMessage(const std::string& message, domain::LogLevel level) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    m_hasOutputSinceLastTransition = true;
    if (level == domain::LogLevel::Error) {
        m_ctx.logger->error(message);
    } else if (level == domain::LogLevel::Warning) {
        m_ctx.logger->warn(message);
    } else {
        m_ctx.logger->info(message);
    }
}

int DownloadScreen::onPauseRequested(bool isMultiVideo) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    std::vector<std::string> pauseOptions = {
        m_ctx.loc->tr("Resume download", "Продолжить скачивание"),
        m_ctx.loc->tr("Cancel download of current video", "Отменить скачивание текущего видео")
    };
    if (isMultiVideo) {
        pauseOptions.push_back(m_ctx.loc->tr("Cancel download of all videos", "Отменить скачивание всех видео"));
    }

    std::string countStr = std::to_string(pauseOptions.size());
    std::string hint1 = m_ctx.loc->tr(
        " Arrows to select - Enter to confirm",
        " Стрелочки для выбор - ввод для подтверждения"
    );
    std::string hint2 = m_ctx.loc->tr(
        " Or numbers 1-" + countStr + " to select without Enter",
        " Либо цифрами 1-" + countStr + " выбор без подтверждения"
    );

    return Menu::inlineMenuSelect(pauseOptions, hint1, hint2);
}

bool DownloadScreen::onPlaylistDelay(int completedCount, int durationSeconds) {
    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }
    auto trFunc = [this](const std::string& en, const std::string& ru) {
        return m_ctx.loc->tr(en, ru);
    };
    return ProgressRenderer::renderDelayCountdown(completedCount, durationSeconds, trFunc);
}

void DownloadScreen::start(const std::string& url,
                           const std::string& startIdx,
                           const std::string& endIdx,
                           bool isPlaylist,
                           bool retry) {
    std::string u = url, s = startIdx, e = endIdx;
    bool isPl = isPlaylist;

    if (u.empty()) {
        if (!retry) {
            core::Console::clear();
        } else {
            std::cout << "\n";
        }

        core::Console::print("============================================", core::ConsoleColor::Cyan);
        core::Console::print(retry ? m_ctx.loc->tr(" RETRYING DOWNLOAD", " ПОВТОРНАЯ ПОПЫТКА СКАЧИВАНИЯ") :
                                     m_ctx.loc->tr(" STARTING DOWNLOAD", " НАЧАЛО СКАЧИВАНИЯ"), core::ConsoleColor::Cyan);
        core::Console::print("============================================", core::ConsoleColor::Cyan);
        std::cout << "\n" << m_ctx.loc->tr("Enter URL (Paste: Ctrl+V, Cancel: ESC):\n", "Введите URL (Вставка: Ctrl+V, Отмена: ESC):\n");

        if (!InputReader::inputLineWithEscape(u, "> ")) {
            m_ctx.logger->warn(m_ctx.loc->tr("[INFO] Cancelled", "[ИНФО] Отменено"));
            InputReader::waitForKey();
            return;
        }

        u = core::StringUtil::trim(u);
        while (!u.empty() && (u.front() == '"' || u.front() == '\'')) u.erase(u.begin());
        while (!u.empty() && (u.back() == '"' || u.back() == '\'')) u.pop_back();

        if (u.empty()) {
            m_ctx.logger->error(m_ctx.loc->tr("[ERROR] URL cannot be empty!", "[ОШИБКА] URL не может быть пустым!"));
            InputReader::waitForKey();
            return;
        }

        if (u.find("&list=") != std::string::npos && u.find("/playlist?list=") == std::string::npos) {
            std::vector<std::string> plOptions = {
                m_ctx.loc->tr("Download entire playlist", "Скачать весь плейлист"),
                m_ctx.loc->tr("Download only this video", "Скачать только это видео")
            };
            int plChoice = Menu::arrowSelect(
                m_ctx.loc->tr("PLAYLIST DETECTED", "ОБНАРУЖЕН ПЛЕЙЛИСТ"),
                m_ctx.loc->tr("URL contains both a video and a playlist:\nChoose download mode:",
                              "URL содержит и видео, и плейлист:\nВыберите режим скачивания:"),
                plOptions, 0
            );
            if (plChoice < 0) {
                m_ctx.logger->warn(m_ctx.loc->tr("[INFO] Cancelled", "[ИНФО] Отменено"));
                InputReader::waitForKey();
                return;
            }
            if (plChoice == 0) {
                isPl = true;
            } else {
                size_t pos = u.find("&list=");
                if (pos != std::string::npos) u = u.substr(0, pos);
                isPl = false;
            }
        }
        else if (u.find("/playlist?list=") != std::string::npos || u.find("list=") != std::string::npos) {
            isPl = true;
        }
        else {
            isPl = false;
        }

        if (isPl) {
            std::string inp;
            std::cout << "\n" << m_ctx.loc->tr("Start index in playlist (Enter=first, ESC to skip):\n",
                                               "Начальный номер в плейлисте (Enter=с начала, ESC=пропустить):\n");
            if (InputReader::inputLineWithEscape(inp, m_ctx.loc->tr("Your choice: ", "Ваш выбор: "))) {
                if (!inp.empty()) s = inp;
            }

            std::cout << "\n" << m_ctx.loc->tr("End index in playlist (Enter=last, ESC to skip):\n",
                                               "Конечный номер в плейлисте (Enter=до конца, ESC=пропустить):\n");
            if (InputReader::inputLineWithEscape(inp, m_ctx.loc->tr("Your choice: ", "Ваш выбор: "))) {
                if (!inp.empty()) e = inp;
            }
        }
        m_skippedVideos.clear();
    }

    if (!retry) {
        core::Console::clear();
    }
    core::Console::print("============================================", core::ConsoleColor::Cyan);
    core::Console::print(retry ? m_ctx.loc->tr(" RETRYING DOWNLOAD...", " ПОВТОРНАЯ ПОПЫТКА СКАЧИВАНИЯ...") :
                                 m_ctx.loc->tr(" STARTING DOWNLOAD...", " НАЧАЛО СКАЧИВАНИЯ..."), core::ConsoleColor::Cyan);
    core::Console::print("============================================", core::ConsoleColor::Cyan);
    std::cout << std::endl;

    domain::DownloadOptions opts;
    opts.url = u;
    opts.startIndex = s;
    opts.endIndex = e;
    opts.isPlaylist = isPl;

    m_ctx.dependencyManager->refreshStatus();
    m_progressActive = false;
    m_lastMediaItemIndex = 0;
    auto result = m_ctx.ytDlpService->execute(m_ctx.config, opts, m_ctx.dependencyManager->getStatus(), this);

    if (m_progressActive) {
        std::cout << "\n";
        m_progressActive = false;
    }

    if (result.error == domain::ErrorType::CancelledByUser) {
        InputReader::waitForKey();
        return;
    }

    if (result.error == domain::ErrorType::SkipCurrentVideo) {
        if (isPl) {
            int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : std::stoi(s));
            int nextIndex = currentIndex + 1;
            if (result.totalItems > 0 && nextIndex > result.totalItems) {
                m_ctx.logger->info(m_ctx.loc->tr("[OK] All remaining playlist items completed.", "[OK] Все оставшиеся видео плейлиста завершены."));
                InputReader::waitForKey();
                return;
            }
            start(u, std::to_string(nextIndex), e, true, true);
            return;
        }
        InputReader::waitForKey();
        return;
    }

    if (result.error == domain::ErrorType::RateLimit) {
        m_ctx.logger->error(m_ctx.loc->tr("[ERROR] YouTube rate-limited requests. Waiting up to 1h (ESC to cancel)...",
                                          "[ОШИБКА] Превышен лимит запросов YouTube. Ожидание до 1 часа (ESC для отмены)..."));
        bool cancelled = false;
        for (int sec = 0; sec < 3660 && !cancelled; ++sec) {
            if (core::Keyboard::hasKey() && core::Keyboard::readWideChar() == 27) {
                cancelled = true;
            } else {
                Sleep(1000);
            }
            if (sec > 0 && sec % 300 == 0) {
                m_ctx.logger->warn(m_ctx.loc->tr("[INFO] ~", "[ИНФО] ~") + std::to_string((3660 - sec) / 60) +
                                   m_ctx.loc->tr(" min remaining (ESC to cancel)", " мин осталось (ESC для отмены)"));
            }
        }
        if (cancelled) {
            m_ctx.logger->warn(m_ctx.loc->tr("[INFO] Cancelled by user.", "[ИНФО] Отменено пользователем."));
            InputReader::waitForKey();
            return;
        }
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : std::stoi(s));
        start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, false);
        return;
    }

    if (result.error == domain::ErrorType::PoTokenBotguard) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        while (true) {
            int sel = Dialogs::showPoTokenRecoveryMenu(isPl, m_ctx.paths->getPluginsDir(), *m_ctx.loc);
            if (sel == 0) {
                CookieEditorScreen ce(m_ctx, false);
                if (ce.run() == CookieEditorResult::Continue) {
                    start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
                    return;
                }
            } else if (sel == 1) {
                core::FileSystem::openUrlInBrowser("https://github.com/yt-dlp/yt-dlp/wiki/PO-Token-Guide");
                m_ctx.logger->info(m_ctx.loc->tr("[OK] PO-Token Guide opened in browser!", "[OK] Руководство PO-Token Guide открыто в браузере!"));
                Sleep(2000);
            } else if (sel == 2) {
                core::FileSystem::openFolderInExplorer(m_ctx.paths->getPluginsDir());
                m_ctx.logger->info(m_ctx.loc->tr("[OK] Plugins folder opened in Explorer!", "[OK] Папка плагинов открыта в Проводнике!"));
                Sleep(2000);
            } else if (sel == 3) {
                start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
                return;
            } else if (isPl && sel == 4) {
                m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex) +
                                          m_ctx.loc->tr(" [PO-Token Missing]", " [PO-Token отсутствует]"));
                start(u, std::to_string(currentIndex + 1), e, true, true);
                return;
            } else {
                return;
            }
        }
    }

    if (result.error == domain::ErrorType::AuthenticationRequired || result.error == domain::ErrorType::AgeRestricted) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        while (true) {
            int sel = Dialogs::showAuthRequiredMenu(isPl, result.error == domain::ErrorType::AgeRestricted, *m_ctx.loc);
            if (sel == 0) {
                CookieEditorScreen ce(m_ctx, false);
                if (ce.run() == CookieEditorResult::Continue) {
                    start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
                    return;
                }
            } else if (sel == 1) {
                core::FileSystem::openUrlInBrowser("https://github.com/yt-dlp/yt-dlp/wiki/FAQ#how-do-i-pass-cookies-to-yt-dlp");
                m_ctx.logger->info(m_ctx.loc->tr("[OK] Cookies Guide opened in browser!", "[OK] Инструкция по куки открыта в браузере!"));
                Sleep(2000);
            } else if (sel == 2) {
                start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
                return;
            } else if (isPl && sel == 3) {
                m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex) +
                                          m_ctx.loc->tr(" [Auth Required]", " [Требуется авторизация]"));
                start(u, std::to_string(currentIndex + 1), e, true, true);
                return;
            } else {
                return;
            }
        }
    }

    if (result.error == domain::ErrorType::DrmProtected) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        int sel = Dialogs::showDrmErrorMenu(isPl, *m_ctx.loc);
        if (sel == 0) {
            core::FileSystem::openUrlInBrowser("https://github.com/yt-dlp/yt-dlp#can-you-add-support-for-this-feature-request");
            Sleep(2000);
        } else if (isPl && sel == 1) {
            m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex) +
                                      m_ctx.loc->tr(" [DRM Protected]", " [Защита DRM]"));
            start(u, std::to_string(currentIndex + 1), e, true, true);
        }
        return;
    }

    if (result.error == domain::ErrorType::VideoUnavailable) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        int sel = Dialogs::showUnavailableMenu(isPl, *m_ctx.loc);
        if (sel == 0) {
            CookieEditorScreen ce(m_ctx, false);
            if (ce.run() == CookieEditorResult::Continue) {
                start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
            }
        } else if (isPl && sel == 1) {
            m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex) +
                                      m_ctx.loc->tr(" [Unavailable]", " [Недоступно]"));
            start(u, std::to_string(currentIndex + 1), e, true, true);
        }
        return;
    }

    if (result.error == domain::ErrorType::FormatNotAvailable) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        int sel = Dialogs::showFormatErrorMenu(isPl, *m_ctx.loc);
        if (sel == 0) {
            std::string prevRes = m_ctx.config.videoResolution;
            m_ctx.config.videoResolution = "Auto";
            start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
            m_ctx.config.videoResolution = prevRes;
        } else if (isPl && sel == 1) {
            m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex) +
                                      m_ctx.loc->tr(" [Format Missing]", " [Формат недоступен]"));
            start(u, std::to_string(currentIndex + 1), e, true, true);
        }
        return;
    }

    if (result.error == domain::ErrorType::NetworkError) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        int sel = Dialogs::showNetworkErrorMenu(isPl, *m_ctx.loc);
        if (sel == 0) {
            start(u, isPl ? std::to_string(currentIndex) : s, e, isPl, true);
        } else if (isPl && sel == 1) {
            m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex) +
                                      m_ctx.loc->tr(" [Network Error]", " [Сетевая ошибка]"));
            start(u, std::to_string(currentIndex + 1), e, true, true);
        }
        return;
    }

    if (result.error == domain::ErrorType::LiveStreamNotStarted) {
        if (isPl) {
            int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
            m_ctx.logger->warn(m_ctx.loc->tr("[INFO] Video ", "[ИНФО] Видео ") + std::to_string(currentIndex) +
                               m_ctx.loc->tr(" is a scheduled live stream. Skipping...", " является запланированной трансляцией. Пропуск..."));
            Sleep(2000);
            start(u, std::to_string(currentIndex + 1), e, true, true);
            return;
        }
        m_ctx.logger->error(m_ctx.loc->tr("[ERROR] This video is a live stream that has not started yet.",
                                          "[ОШИБКА] Это видео является трансляцией, которая ещё не началась."));
        InputReader::waitForKey();
        return;
    }

    if (!result.success && isPl) {
        int currentIndex = (result.lastProcessedItem > 0) ? result.lastProcessedItem : (s.empty() ? 1 : 1);
        int nextIndex = currentIndex + 1;
        bool isLast = (result.totalItems > 0 && currentIndex >= result.totalItems);
        if (!e.empty()) {
            try { if (currentIndex >= std::stoi(e)) isLast = true; } catch (...) {}
        }
        if (isLast) {
            std::string targetDir = !result.savedDirectory.empty() ? result.savedDirectory : m_ctx.config.downloadPath;
            while (!targetDir.empty() && (targetDir.back() == '\\' || targetDir.back() == '/')) {
                targetDir.pop_back();
            }
            std::cout << "\n";
            core::Console::print(m_ctx.loc->tr("[OK] Videos processed and saved to:", "[OK] Видео обработаны и сохранены по пути:"), core::ConsoleColor::Green);
            core::Console::print("\"" + targetDir + "\"", core::ConsoleColor::Green);
            InputReader::waitForKey();
            return;
        }
        m_skippedVideos.push_back(m_ctx.loc->tr("Item ", "Элемент ") + std::to_string(currentIndex));
        m_ctx.logger->error(m_ctx.loc->tr("[ERROR] Video ", "[ОШИБКА] Видео ") + std::to_string(currentIndex) +
                            m_ctx.loc->tr(" could not be downloaded (skipping).", " не удалось скачать (пропуск)."));
        Sleep(3000);
        start(u, std::to_string(nextIndex), e, true, true);
        return;
    }

    if (!result.success) {
        int selErr = Dialogs::showSingleVideoErrorMenu(*m_ctx.loc);
        if (selErr == 0) {
            CookieEditorScreen ce(m_ctx, false);
            if (ce.run() == CookieEditorResult::Continue) {
                start(u, s, e, isPl, true);
            }
        } else if (selErr == 1) {
            m_ctx.dependencyManager->installYtDlp();
            start(u, s, e, isPl, true);
        } else if (selErr == 2) {
            core::FileSystem::openUrlInBrowser("https://github.com/yt-dlp/yt-dlp/wiki/FAQ");
            Sleep(2000);
            start(u, s, e, isPl, true);
        } else if (selErr == 3) {
            start(u, s, e, isPl, true);
        }
        return;
    }

    std::string targetDir = !result.savedDirectory.empty() ? result.savedDirectory : m_ctx.config.downloadPath;
    while (!targetDir.empty() && (targetDir.back() == '\\' || targetDir.back() == '/')) {
        targetDir.pop_back();
    }
    std::cout << "\n";
    core::Console::print(m_ctx.loc->tr("[OK] Videos processed and saved to:", "[OK] Видео обработаны и сохранены по пути:"), core::ConsoleColor::Green);
    core::Console::print("\"" + targetDir + "\"", core::ConsoleColor::Green);
    InputReader::waitForKey();
}

} // namespace mrcli::ui::screens
