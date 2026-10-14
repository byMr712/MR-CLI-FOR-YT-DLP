#include "SettingsScreen.hpp"
#include "CookieEditorScreen.hpp"
#include "src/ui/Menu.hpp"
#include "src/ui/Dialogs.hpp"
#include "src/ui/InputReader.hpp"
#include "src/core/Console.hpp"
#include "src/core/FileSystem.hpp"
#include "src/core/Keyboard.hpp"
#include "src/core/StringUtil.hpp"
#include <iostream>

namespace mrcli::ui::screens {

SettingsScreen::SettingsScreen(app::AppContext& ctx) : m_ctx(ctx) {}

void SettingsScreen::show() {
    auto& loc = *m_ctx.loc;

    while (true) {
        std::vector<std::string> opts = {
            loc.tr("Download location: ", "Папка для скачивания: ") + m_ctx.config.downloadPath,
            loc.tr("Video Quality: ", "Качество видео: ") + m_ctx.config.videoResolution + " " + m_ctx.config.videoFps + "fps " + m_ctx.config.videoFormat,
            loc.tr("Audio Quality: ", "Качество аудио: ") + m_ctx.config.audioFormat,
            loc.tr("Codec Recompiler: [", "Перекодировщик кодеков: [") + (m_ctx.config.codecRecompiler ? loc.tr("ON", "ВКЛ") : loc.tr("OFF", "ВЫКЛ")) + "]",
            loc.tr("Download Archive: [", "История загрузок (Архив): [") + (m_ctx.config.useArchive ? loc.tr("ON", "ВКЛ") : loc.tr("OFF", "ВЫКЛ")) + "]",
            loc.tr("Download Only Audio: [", "Скачивать только аудио: [") + (m_ctx.config.onlyAudio ? loc.tr("ON", "ВКЛ") : loc.tr("OFF", "ВЫКЛ")) + "]",
            loc.tr("Download Only Video: [", "Скачивать только видео: [") + (m_ctx.config.onlyVideo ? loc.tr("ON", "ВКЛ") : loc.tr("OFF", "ВЫКЛ")) + "]",
            loc.tr("Playlist delay: [", "Задержка плейлистов: [") + (m_ctx.config.playlistDelayEnabled ? loc.tr("ON", "ВКЛ") : loc.tr("OFF", "ВЫКЛ")) + "] " +
                loc.tr("every ", "каждые ") + std::to_string(m_ctx.config.playlistDelayInterval) + loc.tr(" vids / ", " вид. / ") + std::to_string(m_ctx.config.playlistDelayDuration) + loc.tr("s", "с"),
            loc.tr("Clear Download Archive", "Очистить историю загрузок"),
            loc.tr("Cookie Editor", "Редактор куки"),
            loc.tr("Update components", "Обновить компоненты"),
            loc.tr("Return to Main Menu (ESC)", "Вернуться в главное меню (ESC)")
        };

        std::vector<char> actions = { '1', '2', '3', '4', '5', '6', '7', '8', '9', 'c', 'u', '0' };

        core::Console::clear();
        core::Console::print("========================================", core::ConsoleColor::Cyan);
        core::Console::print(" " + loc.tr("SETTINGS", "НАСТРОЙКИ"), core::ConsoleColor::Cyan);
        core::Console::print("========================================", core::ConsoleColor::Cyan);
        std::cout << "========================================\n";

        int selected = 0;
        int actionIndex = Menu::arrowSelect(loc.tr("SETTINGS", "НАСТРОЙКИ"), "", opts, selected);
        if (actionIndex < 0 || actionIndex >= static_cast<int>(actions.size())) {
            return;
        }

        char ch = actions[actionIndex];
        switch (ch) {
        case '1': {
            std::wstring folderTitle = (m_ctx.config.language == domain::Language::Russian) ?
                L"Выберите папку для сохранения" : L"Select folder";
            std::string p = Dialogs::openFolderDialog(folderTitle);
            if (!p.empty()) {
                m_ctx.config.downloadPath = p;
                core::FileSystem::createDirectories(m_ctx.config.downloadPath);
                m_ctx.saveConfig();
                m_ctx.logger->info(loc.tr("[OK] Download location updated!", "[OK] Папка загрузки обновлена!"));
            } else {
                m_ctx.logger->warn(loc.tr("[INFO] Not changed", "[ИНФО] Не изменено"));
            }
            InputReader::waitForKey();
            break;
        }
        case '2': selectVideoQuality(); break;
        case '3': selectAudioQuality(); break;
        case '4': codecRecompilerMenu(); break;
        case '5': toggleUseArchive(); break;
        case '6': toggleOnlyAudio(); break;
        case '7': toggleOnlyVideo(); break;
        case '8': playlistDelayMenu(); break;
        case '9': clearArchiveMenu(); break;
        case 'c': {
            CookieEditorScreen ce(m_ctx, true);
            ce.run();
            m_ctx.saveConfig();
            break;
        }
        case 'u': updateComponentsMenu(); break;
        case '0': return;
        default: break;
        }
    }
}

void SettingsScreen::selectVideoQuality() {
    auto& loc = *m_ctx.loc;

    std::vector<std::string> resOpts = {
        "2160p (4K UHD)",
        "1440p (2K QHD)",
        "1080p (FullHD)",
        "720p (HD)",
        "480p",
        "360p",
        loc.tr("Auto (Best available)", "Авто (Наилучшее)")
    };
    int selRes = Menu::arrowSelect(loc.tr("VIDEO RESOLUTION", "РАЗРЕШЕНИЕ ВИДЕО"), "", resOpts, 2);
    if (selRes >= 0) {
        static const char* resValues[] = { "2160", "1440", "1080", "720", "480", "360", "Auto" };
        m_ctx.config.videoResolution = resValues[selRes];
    }

    std::vector<std::string> fpsOpts = { "60 fps", "30 fps" };
    int selFps = Menu::arrowSelect(loc.tr("FRAME RATE (FPS)", "ЧАСТОТА КАДРОВ (FPS)"), "", fpsOpts, 0);
    if (selFps >= 0) {
        m_ctx.config.videoFps = (selFps == 0) ? "60" : "30";
    }

    std::vector<std::string> fmtOpts = {
        "MP4 (H.264 / AVC)",
        "MP4 (AV1)",
        "WEBM (VP9)",
        "WEBM (AV1)"
    };
    int selFmt = Menu::arrowSelect(loc.tr("VIDEO CONTAINER & CODEC", "КОНТЕЙНЕР И КОДЕК"), "", fmtOpts, 0);
    if (selFmt >= 0) {
        static const char* fmtValues[] = { "MP4(H.264)", "MP4(AV1)", "WEBM(VP9)", "WEBM(AV1)" };
        m_ctx.config.videoFormat = fmtValues[selFmt];
    }

    m_ctx.saveConfig();
}

void SettingsScreen::selectAudioQuality() {
    auto& loc = *m_ctx.loc;

    std::vector<std::string> opts = {
        "M4A (AAC) - " + loc.tr("Recommended", "Рекомендуется"),
        "Opus (WEBM)",
        "Vorbis (OGG)"
    };
    int sel = Menu::arrowSelect(loc.tr("AUDIO FORMAT", "ФОРМАТ АУДИО"), "", opts, 0);
    if (sel >= 0) {
        static const char* audioValues[] = { "M4A(AAC)", "Opus", "Vorbis" };
        m_ctx.config.audioFormat = audioValues[sel];
        m_ctx.saveConfig();
    }
}

void SettingsScreen::codecRecompilerMenu() {
    auto& loc = *m_ctx.loc;
    std::string status = loc.tr("Status: [", "Статус: [") +
        (m_ctx.config.codecRecompiler ? loc.tr("ENABLED", "ВКЛЮЧЕНО") : loc.tr("DISABLED", "ВЫКЛЮЧЕНО")) + "]";
    std::string desc = "\n" + status + "\n\n" +
        loc.tr("When enabled, yt-dlp and FFmpeg re-encode downloaded videos\n(including 4K/VP9/AV1) into universal H.264 + AAC MP4 container.\nThis ensures maximum compatibility with older TVs and players.",
               "Когда включено, yt-dlp и FFmpeg перекодируют скачанные видео\n(включая 4K/VP9/AV1) в универсальный MP4-контейнер H.264 + AAC.\nЭто обеспечивает максимальную совместимость со старыми ТВ и плеерами.");

    std::vector<std::string> opts = {
        loc.tr("[ON] - Re-encode to H.264 MP4", "[ВКЛ] - Перекодировать в H.264 MP4"),
        loc.tr("[OFF] - No re-encoding (default)", "[ВЫКЛ] - Без перекодирования (по умолчанию)")
    };

    int sel = Menu::arrowSelect(loc.tr("CODEC RECOMPILER", "ПЕРЕКОДИРОВЩИК КОДЕКОВ"), desc, opts, m_ctx.config.codecRecompiler ? 0 : 1);
    if (sel >= 0) {
        m_ctx.config.codecRecompiler = (sel == 0);
        m_ctx.saveConfig();
    }
}

void SettingsScreen::toggleUseArchive() {
    m_ctx.config.useArchive = !m_ctx.config.useArchive;
    m_ctx.saveConfig();
}

void SettingsScreen::toggleOnlyAudio() {
    m_ctx.config.onlyAudio = !m_ctx.config.onlyAudio;
    if (m_ctx.config.onlyAudio) m_ctx.config.onlyVideo = false;
    m_ctx.saveConfig();
}

void SettingsScreen::toggleOnlyVideo() {
    m_ctx.config.onlyVideo = !m_ctx.config.onlyVideo;
    if (m_ctx.config.onlyVideo) m_ctx.config.onlyAudio = false;
    m_ctx.saveConfig();
}

void SettingsScreen::playlistDelayMenu() {
    auto& loc = *m_ctx.loc;

    while (true) {
        std::string statusStr = m_ctx.config.playlistDelayEnabled ? loc.tr("ENABLED", "ВКЛЮЧЕНА") : loc.tr("DISABLED", "ВЫКЛЮЧЕНА");
        std::string desc = "\n" + loc.tr("Delay for playlist downloads to prevent YouTube HTTP 429 rate limits.\n",
                                          "Задержка для скачивания плейлистов для защиты от блокировок YouTube (HTTP 429).\n") +
            loc.tr("Delay state: ", "Состояние задержки: ") + statusStr + "\n" +
            loc.tr("Enable delay every: ", "Через сколько видео включать: ") + loc.tr("every ", "каждое ") + std::to_string(m_ctx.config.playlistDelayInterval) + loc.tr(" videos", " видео") + "\n" +
            loc.tr("Pause duration: ", "Какое время ожидать: ") + std::to_string(m_ctx.config.playlistDelayDuration) + loc.tr(" seconds", " секунд") + "\n";

        std::vector<std::string> opts = {
            loc.tr("Toggle state: ", "Переключить состояние: ") + (m_ctx.config.playlistDelayEnabled ? loc.tr("[Turn OFF]", "[ВЫКЛЮЧИТЬ]") : loc.tr("[Turn ON]", "[ВКЛЮЧИТЬ]")),
            loc.tr("Change video batch interval (current: every ", "Изменить интервал видео (текущий: каждые ") + std::to_string(m_ctx.config.playlistDelayInterval) + ")",
            loc.tr("Change pause duration (current: ", "Изменить время паузы (текущее: ") + std::to_string(m_ctx.config.playlistDelayDuration) + loc.tr("s)", "с)"),
            loc.tr("Back to settings", "Назад в настройки")
        };

        int sel = Menu::arrowSelect(loc.tr("PLAYLIST DELAY CONFIGURATION", "НАСТРОЙКА ЗАДЕРЖКИ ПЛЕЙЛИСТОВ"), desc, opts, 0);
        if (sel == 0) {
            m_ctx.config.playlistDelayEnabled = !m_ctx.config.playlistDelayEnabled;
            m_ctx.saveConfig();
        } else if (sel == 1) {
            std::cout << "\n" << loc.tr("Enter video interval count (default 10): ", "Введите через сколько видео включать задержку (по умолчанию 10): ");
            std::string inp;
            if (InputReader::inputLineWithEscape(inp) && !inp.empty()) {
                try {
                    int val = std::stoi(inp);
                    if (val > 0) {
                        m_ctx.config.playlistDelayInterval = val;
                        m_ctx.saveConfig();
                    }
                } catch (...) {}
            }
        } else if (sel == 2) {
            std::cout << "\n" << loc.tr("Enter duration in seconds (default 30): ", "Введите время ожидания в секундах (по умолчанию 30): ");
            std::string inp;
            if (InputReader::inputLineWithEscape(inp) && !inp.empty()) {
                try {
                    int val = std::stoi(inp);
                    if (val > 0) {
                        m_ctx.config.playlistDelayDuration = val;
                        m_ctx.saveConfig();
                    }
                } catch (...) {}
            }
        } else {
            break;
        }
    }
}

void SettingsScreen::clearArchiveMenu() {
    auto& loc = *m_ctx.loc;
    std::vector<std::string> opts = {
        loc.tr("Yes, clear archive", "Да, очистить архив"),
        loc.tr("No, cancel", "Нет, отмена")
    };
    int sel = Menu::arrowSelect(loc.tr("CLEAR ARCHIVE", "ОЧИСТИТЬ АРХИВ"),
                                loc.tr("Are you sure you want to clear download archive history?", "Вы уверены, что хотите очистить историю загрузок?"),
                                opts, 1);
    if (sel == 0) {
        m_ctx.archiveService->clear();
        m_ctx.logger->info(loc.tr("[OK] Download archive cleared!", "[OK] Архив загрузок очищен!"));
        InputReader::waitForKey();
    }
}

void SettingsScreen::updateComponentsMenu() {
    auto& loc = *m_ctx.loc;
    std::vector<std::string> opts = {
        loc.tr("Update yt-dlp", "Обновить yt-dlp"),
        loc.tr("Update FFmpeg", "Обновить FFmpeg"),
        loc.tr("Update QuickJS", "Обновить QuickJS"),
        loc.tr("Back", "Назад")
    };
    int sel = Menu::arrowSelect(loc.tr("UPDATE COMPONENTS", "ОБНОВЛЕНИЕ КОМПОНЕНТОВ"), "", opts, 0);
    if (sel == 0) {
        m_ctx.dependencyManager->installYtDlp();
        InputReader::waitForKey();
    } else if (sel == 1) {
        m_ctx.dependencyManager->installFFmpeg();
        InputReader::waitForKey();
    } else if (sel == 2) {
        m_ctx.dependencyManager->installQuickJs();
        InputReader::waitForKey();
    }
}

} // namespace mrcli::ui::screens
