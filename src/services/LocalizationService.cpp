#include "LocalizationService.hpp"
#include "src/core/WinLibs.hpp"

namespace mrcli::services {

LocalizationService::LocalizationService(domain::Language lang)
    : m_currentLang(lang) {
    initStrings();
}

domain::Language LocalizationService::detectSystemLanguage() noexcept {
    WORD langId = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(langId);
    if (primary == LANG_RUSSIAN || primary == LANG_BELARUSIAN || primary == LANG_UKRAINIAN) {
        return domain::Language::Russian;
    }
    return domain::Language::English;
}

std::string LocalizationService::get(StrId id) const {
    auto it = m_strings.find(id);
    if (it != m_strings.end()) {
        return (m_currentLang == domain::Language::Russian) ? it->second.second : it->second.first;
    }
    return {};
}

std::string LocalizationService::tr(std::string_view en, std::string_view ru) const {
    return (m_currentLang == domain::Language::Russian) ? std::string(ru) : std::string(en);
}

void LocalizationService::initStrings() {
    m_strings = {
        { StrId::AppTitle, { "MR CLI FOR YT-DLP", "MR CLI ДЛЯ YT-DLP" } },
        { StrId::MenuDownload, { "Download (Video / Audio / Playlist)", "Скачать (Видео / Аудио / Плейлист)" } },
        { StrId::MenuSettings, { "Settings", "Настройки" } },
        { StrId::MenuHelp, { "Help & Info", "Справка и информация" } },
        { StrId::MenuExit, { "Exit", "Выход" } },
        { StrId::MenuLanguage, { "Language", "Язык" } },
        { StrId::SettingsTitle, { "SETTINGS", "НАСТРОЙКИ" } },
        { StrId::DownloadLocation, { "Download location: [", "Папка для скачивания: [" } },
        { StrId::VideoQuality, { "Video quality: [", "Качество видео: [" } },
        { StrId::AudioQuality, { "Audio quality: [", "Качество аудио: [" } },
        { StrId::CodecRecompiler, { "Codec recompiler: [", "Перекодировщик кодеков: [" } },
        { StrId::SkipArchive, { "Skip already downloaded (archive): [", "Пропускать уже скачанное (архив): [" } },
        { StrId::OnlyAudio, { "Only audio: [", "Только аудио: [" } },
        { StrId::OnlyVideo, { "Only video: [", "Только видео: [" } },
        { StrId::ConfigureDelay, { "Configure delay (playlists): [", "Настроить задержку: [" } },
        { StrId::ClearArchive, { "Clear download history (archive)", "Очистить историю загрузок (архив)" } },
        { StrId::UpdateCookies, { "Update cookies", "Обновить куки" } },
        { StrId::UpdateComponents, { "Update yt-dlp & components", "Обновить yt-dlp и компоненты" } },
        { StrId::ReturnEsc, { "Return (ESC)", "Назад (ESC)" } },
        { StrId::EnterUrl, { "Enter URL (Paste: Ctrl+V, Cancel: ESC):", "Введите URL (Вставка: Ctrl+V, Отмена: ESC):" } },
        { StrId::UrlEmpty, { "URL cannot be empty!", "URL не может быть пустым!" } },
        { StrId::PlaylistDetected, { "PLAYLIST DETECTED", "ОБНАРУЖЕН ПЛЕЙЛИСТ" } },
        { StrId::PlaylistChoiceEntire, { "Download entire playlist", "Скачать весь плейлист" } },
        { StrId::PlaylistChoiceSingle, { "Download only this video", "Скачать только это видео" } },
        { StrId::PlaylistStartIndex, { "Start index in playlist (Enter=first, ESC to skip):", "Начальный номер в плейлисте (Enter=с начала, ESC=пропустить):" } },
        { StrId::PlaylistEndIndex, { "End index in playlist (Enter=last, ESC to skip):", "Конечный номер в плейлисте (Enter=до конца, ESC=пропустить):" } },
        { StrId::StoppedByUser, { "[INFO] Stopped by user.", "[ИНФО] Остановлено пользователем." } },
        { StrId::ResumeDownload, { "Resume download", "Продолжить скачивание" } },
        { StrId::CancelCurrentVideo, { "Cancel download of current video", "Отменить скачивание текущего видео" } },
        { StrId::CancelAllVideos, { "Cancel download of all videos", "Отменить скачивание всех видео" } },
        { StrId::DelayTitle, { "PLAYLIST DELAY CONFIGURATION", "НАСТРОЙКА ЗАДЕРЖКИ ПЛЕЙЛИСТА" } },
        { StrId::DelayDesc, {
            "Delay between videos when downloading playlists.\n"
            "Helps prevent temporary YouTube IP blocks, HTTP 429, and bot detection.",
            "Задержка между видео при скачивании плейлистов.\n"
            "Помогает предотвратить временные блокировки IP, ошибки HTTP 429 и проверки на бота."
        } },
        { StrId::DelayState, { "Delay state: ", "Состояние задержки: " } },
        { StrId::DelayInterval, { "Trigger interval: every ", "Через сколько видео включать: каждое " } },
        { StrId::DelayDuration, { "Wait duration: ", "Какое время ожидать: " } },
        { StrId::Enabled, { "ENABLED", "ВКЛЮЧЕНО" } },
        { StrId::Disabled, { "DISABLED", "ВЫКЛЮЧЕНО" } },
        { StrId::PressAnyKey, { "Press any key...", "Нажмите любую клавишу..." } },
        { StrId::Copied, { "[OK] Copied: ", "[OK] Скопирован: " } },
        { StrId::ExtractingComponents, { "[INFO] Extracting components...", "[ИНФО] Распаковка компонентов..." } },
        { StrId::InstallingYtDlp, { "[INFO] Installing yt-dlp (~20MB)...", "[ИНФО] Установка yt-dlp (~20MB)..." } },
        { StrId::InstallingFFmpeg, { "[INFO] Installing FFmpeg (~160MB)...", "[ИНФО] Установка FFmpeg (~160MB)..." } },
        { StrId::InstallingQuickJS, { "[INFO] Installing QuickJS (~1MB)...", "[ИНФО] Установка QuickJS (~1MB)..." } },
        { StrId::MergeFailedManual, { "\n[INFO] Merge failed. Trying manual FFmpeg merge...", "\n[ИНФО] Ошибка слияния. Попытка ручного слияния через FFmpeg..." } },
        { StrId::ManualMergeRun, { "[INFO] Running manual merge: ", "[ИНФО] Запуск ручного слияния: " } },
        { StrId::ManualMergeSuccess, { "[OK] Manual merge successful!", "[OK] Ручное слияние успешно завершено!" } },
        { StrId::FinalFile, { "[OK] Final file: ", "[OK] Итоговый файл: " } },
        { StrId::MergedFileCreatedAs, { "[WARNING] Merged file created as: ", "[ПРЕДУПРЕЖДЕНИЕ] Объединённый файл создан как: " } },
        { StrId::CurrentVideoCancelled, { "\n[INFO] Current video cancelled. Skipping to next...", "\n[ИНФО] Скачивание текущего видео отменено. Переход к следующему..." } },
        { StrId::VideoCancelledByUser, { "\n[INFO] Video download cancelled by user.", "\n[ИНФО] Скачивание видео отменено пользователем." } },
        { StrId::AllVideosCancelledByUser, { "\n[INFO] Download of all videos cancelled by user.", "\n[ИНФО] Скачивание всех видео отменено пользователем." } },
        { StrId::ResumingDownload, { "\n[INFO] Resuming download...", "\n[ИНФО] Возобновление скачивания..." } }
    };
}

} // namespace mrcli::services
