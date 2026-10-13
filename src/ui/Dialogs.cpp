#include "Dialogs.hpp"
#include "Menu.hpp"
#include "src/core/WinLibs.hpp"
#include "src/core/StringUtil.hpp"
#include "src/core/Console.hpp"
#include <shobjidl.h>
#include <commdlg.h>
#include <iostream>

namespace mrcli::ui {

std::string Dialogs::openFolderDialog(const std::wstring& title) {
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) return "";

    DWORD dwOptions;
    pfd->GetOptions(&dwOptions);
    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    pfd->SetTitle(title.c_str());

    hr = pfd->Show(nullptr);
    if (FAILED(hr)) { pfd->Release(); return ""; }

    IShellItem* psi = nullptr;
    hr = pfd->GetResult(&psi);
    if (FAILED(hr)) { pfd->Release(); return ""; }

    wchar_t* pPath = nullptr;
    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
    psi->Release();
    pfd->Release();

    if (FAILED(hr) || !pPath) return "";

    std::wstring result(pPath);
    CoTaskMemFree(pPath);
    if (!result.empty() && result.back() != L'\\' && result.back() != L'/') {
        result += L'\\';
    }
    return core::StringUtil::wstringToUtf8(result);
}

std::string Dialogs::openFileDialog(bool isRussian) {
    OPENFILENAMEW ofn = { 0 };
    wchar_t fn[MAX_PATH] = L"";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetConsoleWindow();
    ofn.lpstrFilter = isRussian ?
        L"Текстовые файлы (*.txt)\0*.txt\0Все файлы (*.*)\0*.*\0" :
        L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    std::wstring dlgTitle = isRussian ? L"Выберите файл куки" : L"Select cookies file";
    ofn.lpstrTitle = dlgTitle.c_str();

    if (GetOpenFileNameW(&ofn)) {
        return core::StringUtil::wstringToUtf8(std::wstring(fn));
    }
    return "";
}

int Dialogs::showPoTokenRecoveryMenu(bool isPlaylist, const std::string& pluginsPath, services::LocalizationService& loc) {
    std::string potDesc = loc.tr(
        "YouTube blocked access or skipped formats because a PO Token is missing!\n\n"
        "YouTube requires a valid Proof-of-Origin (PO) Token to confirm requests originate\n"
        "from a genuine browser. Without it, video formats may result in HTTP 403 Forbidden\n"
        "or 'Sign in to confirm you're not a bot'.\n\n"
        "Recommended solutions:\n"
        "  1. Install a POT Provider plugin (e.g. bgutil-ytdlp-pot-provider) into:\n"
        "     " + pluginsPath + "\n"
        "  2. Import / Update account cookies (cookies.txt)\n"
        "  3. Official guide: https://github.com/yt-dlp/yt-dlp/wiki/PO-Token-Guide",

        "YouTube заблокировал доступ или пропустил форматы из-за отсутствия PO-токена!\n\n"
        "YouTube требует подтверждение подлинности (Proof-of-Origin Token) для защиты от ботов.\n"
        "Без токена форматы видео недоступны, выдают ошибку HTTP 403 Forbidden\n"
        "или требование 'Подтвердите, что вы не бот'.\n\n"
        "Рекомендуемые решения:\n"
        "  1. Установить плагин генератора токенов (например, bgutil-ytdlp-pot-provider) в папку:\n"
        "     " + pluginsPath + "\n"
        "  2. Обновить или загрузить куки аккаунта (cookies.txt)\n"
        "  3. Официальное руководство: https://github.com/yt-dlp/yt-dlp/wiki/PO-Token-Guide"
    );

    std::vector<std::string> options;
    if (isPlaylist) {
        options = {
            loc.tr("Update / Import cookies", "Обновить / Загрузить куки"),
            loc.tr("Open PO-Token Guide in browser", "Открыть PO-Token Guide в браузере"),
            loc.tr("Open plugins folder in Explorer", "Открыть папку плагинов в Проводнике"),
            loc.tr("Retry downloading this item", "Повторить попытку скачивания этого видео"),
            loc.tr("Skip this item and continue playlist", "Пропустить это видео и продолжить"),
            loc.tr("Abort playlist download", "Прервать скачивание плейлиста")
        };
    } else {
        options = {
            loc.tr("Update / Import cookies", "Обновить / Загрузить куки"),
            loc.tr("Open PO-Token Guide in browser", "Открыть PO-Token Guide в браузере"),
            loc.tr("Open plugins folder in Explorer", "Открыть папку плагинов в Проводнике"),
            loc.tr("Retry download", "Повторить попытку скачивания"),
            loc.tr("Cancel / Return to menu", "Отмена / Вернуться в меню")
        };
    }

    return Menu::arrowSelect(loc.tr("PO-TOKEN / BOTGUARD REQUIRED", "ТРЕБУЕТСЯ PO-ТОКЕН / ЗАЩИТА BOTGUARD"),
                             potDesc, options, 0);
}

int Dialogs::showAuthRequiredMenu(bool isPlaylist, bool isAge, services::LocalizationService& loc) {
    std::string reason = (!isAge) ?
        loc.tr("YouTube requires verification (Login required / Bot check)!\n"
               "Please update cookies or use an account cookie file.\n"
               "Cookies Guide: https://github.com/yt-dlp/yt-dlp/wiki/FAQ#how-do-i-pass-cookies-to-yt-dlp",
               "YouTube требует подтверждения (Требуется вход / Проверка на бота)!\n"
               "Пожалуйста, обновите куки или используйте файл куки аккаунта.\n"
               "Инструкция по куки: https://github.com/yt-dlp/yt-dlp/wiki/FAQ#how-do-i-pass-cookies-to-yt-dlp") :
        loc.tr("This video has age restriction!\n"
               "Please use account cookies with confirmed 18+ age access.\n"
               "Cookies Guide: https://github.com/yt-dlp/yt-dlp/wiki/FAQ#how-do-i-pass-cookies-to-yt-dlp",
               "Это видео имеет возрастное ограничение!\n"
               "Пожалуйста, используйте куки аккаунта с подтверждённым возрастом 18+.\n"
               "Инструкция по куки: https://github.com/yt-dlp/yt-dlp/wiki/FAQ#how-do-i-pass-cookies-to-yt-dlp");

    std::vector<std::string> options;
    if (isPlaylist) {
        options = {
            loc.tr("Update / Import cookies", "Обновить / Загрузить куки"),
            loc.tr("Open Cookies Guide in browser", "Открыть инструкцию по куки в браузере"),
            loc.tr("Retry download for this item", "Повторить попытку для этого видео"),
            loc.tr("Skip this item and continue playlist", "Пропустить это видео и продолжить"),
            loc.tr("Abort playlist download", "Прервать скачивание плейлиста")
        };
    } else {
        options = {
            loc.tr("Update / Import cookies", "Обновить / Загрузить куки"),
            loc.tr("Open Cookies Guide in browser", "Открыть инструкцию по куки в браузере"),
            loc.tr("Retry download", "Повторить попытку скачивания"),
            loc.tr("Cancel / Return to menu", "Отмена / Вернуться в меню")
        };
    }

    return Menu::arrowSelect(loc.tr("AUTHENTICATION REQUIRED", "ТРЕБУЕТСЯ АВТОРИЗАЦИЯ"), reason, options, 0);
}

int Dialogs::showDrmErrorMenu(bool isPlaylist, services::LocalizationService& loc) {
    std::string drmDesc = loc.tr(
        "This video is protected by DRM (Digital Rights Management) encryption!\n\n"
        "yt-dlp and this program cannot download DRM-protected streams\n"
        "(e.g., paid YouTube Movies, Primetime channels, Netflix, etc.).\n"
        "yt-dlp intentionally does not circumvent DRM protection.\n"
        "DRM Policy: https://github.com/yt-dlp/yt-dlp#can-you-add-support-for-this-feature-request",

        "Это видео защищено шифрованием DRM (Digital Rights Management)!\n\n"
        "yt-dlp и данная программа принципиально не поддерживают скачивание DRM-контента\n"
        "(например, платные фильмы YouTube, трансляции по платной подписке и т.д.).\n"
        "yt-dlp намеренно не осуществляет обход или дешифровку DRM.\n"
        "Справка yt-dlp: https://github.com/yt-dlp/yt-dlp#can-you-add-support-for-this-feature-request"
    );

    std::vector<std::string> options;
    if (isPlaylist) {
        options = {
            loc.tr("Open DRM FAQ in browser", "Открыть справку по DRM в браузере"),
            loc.tr("Skip this item and continue playlist", "Пропустить это видео и продолжить"),
            loc.tr("Abort playlist download", "Прервать скачивание плейлиста")
        };
    } else {
        options = {
            loc.tr("Open DRM FAQ in browser", "Открыть справку по DRM в браузере"),
            loc.tr("Return to menu", "Вернуться в главное меню")
        };
    }

    return Menu::arrowSelect(loc.tr("DRM PROTECTED CONTENT", "ЗАЩИТА DRM (WIDEVINE)"), drmDesc, options, 0);
}

int Dialogs::showUnavailableMenu(bool isPlaylist, services::LocalizationService& loc) {
    std::string desc = loc.tr(
        "The requested video is unavailable!\n\n"
        "Possible causes:\n"
        "  - The video is private, removed, or members-only\n"
        "  - Region restricted (uploader made it unavailable in your country)\n"
        "  - Channel account suspended or hidden\n\n"
        "If your account has access to this video, try updating cookies.",

        "Запрошенное видео недоступно!\n\n"
        "Возможные причины:\n"
        "  - Видео является приватным, удалено или доступно только спонсорам\n"
        "  - Региональная блокировка (автор ограничил просмотр в вашей стране)\n"
        "  - Канал заблокирован или видео скрыто\n\n"
        "Если у вашего аккаунта есть доступ к этому видео, попробуйте обновить куки."
    );

    std::vector<std::string> options;
    if (isPlaylist) {
        options = {
            loc.tr("Update / Import cookies", "Обновить / Загрузить куки"),
            loc.tr("Skip this item and continue playlist", "Пропустить это видео и продолжить"),
            loc.tr("Abort playlist download", "Прервать скачивание плейлиста")
        };
    } else {
        options = {
            loc.tr("Update / Import cookies", "Обновить / Загрузить куки"),
            loc.tr("Return to menu", "Вернуться в главное меню")
        };
    }

    return Menu::arrowSelect(loc.tr("VIDEO UNAVAILABLE", "ВИДЕО НЕДОСТУПНО"), desc, options, 0);
}

int Dialogs::showFormatErrorMenu(bool isPlaylist, services::LocalizationService& loc) {
    std::string desc = loc.tr(
        "The requested video format or resolution is not available!\n\n"
        "The selected quality (resolution, FPS, or codec) does not exist for this video.\n"
        "You can retry with the best available format automatically, or adjust quality in Settings.",

        "Запрошенный формат или разрешение недоступны для этого видео!\n\n"
        "Выбранная комбинация разрешения, FPS или кодека отсутствует у этого ролика.\n"
        "Вы можете скачать в лучшем доступном качестве автоматически или изменить настройки."
    );

    std::vector<std::string> options;
    if (isPlaylist) {
        options = {
            loc.tr("Retry with best available format", "Скачать в наилучшем доступном качестве"),
            loc.tr("Skip this item and continue playlist", "Пропустить это видео и продолжить"),
            loc.tr("Abort playlist download", "Прервать скачивание плейлиста")
        };
    } else {
        options = {
            loc.tr("Retry with best available format", "Скачать в наилучшем доступном качестве"),
            loc.tr("Return to menu", "Вернуться в главное меню")
        };
    }

    return Menu::arrowSelect(loc.tr("FORMAT NOT AVAILABLE", "ФОРМАТ НЕДОСТУПЕН"), desc, options, 0);
}

int Dialogs::showNetworkErrorMenu(bool isPlaylist, services::LocalizationService& loc) {
    std::string desc = loc.tr(
        "A network connection error occurred while contacting YouTube!\n\n"
        "The connection was dropped or timed out. Please check your internet\n"
        "connection, VPN/proxy, or DNS settings.",

        "Произошла сетевая ошибка при подключении к YouTube!\n\n"
        "Связь оборвалась или превышен интервал ожидания. Пожалуйста,\n"
        "проверьте подключение к интернету, VPN/прокси или DNS."
    );

    std::vector<std::string> options;
    if (isPlaylist) {
        options = {
            loc.tr("Retry download", "Повторить попытку скачивания"),
            loc.tr("Skip this item and continue playlist", "Пропустить это видео и продолжить"),
            loc.tr("Abort playlist download", "Прервать скачивание плейлиста")
        };
    } else {
        options = {
            loc.tr("Retry download", "Повторить попытку скачивания"),
            loc.tr("Return to menu", "Вернуться в главное меню")
        };
    }

    return Menu::arrowSelect(loc.tr("NETWORK ERROR", "СЕТЕВАЯ ОШИБКА"), desc, options, 0);
}

int Dialogs::showSingleVideoErrorMenu(services::LocalizationService& loc) {
    std::string desc = loc.tr(
        "Download could not be completed!\n\n"
        "Possible reasons:\n"
        "  - Expired / missing cookies or YouTube verification\n"
        "  - Region block or Private / Deleted video\n"
        "  - Network connection issues\n"
        "  - Outdated yt-dlp version\n"
        "Wiki / FAQ: https://github.com/yt-dlp/yt-dlp/wiki/FAQ",

        "Не удалось завершить скачивание!\n\n"
        "Возможные причины:\n"
        "  - Истёкшие или отсутствующие куки / проверка YouTube\n"
        "  - Региональная блокировка или приватное / удалённое видео\n"
        "  - Проблемы с сетевым подключением\n"
        "  - Устаревшая версия yt-dlp\n"
        "Справка / FAQ: https://github.com/yt-dlp/yt-dlp/wiki/FAQ"
    );

    std::vector<std::string> options = {
        loc.tr("Update cookies", "Обновить куки"),
        loc.tr("Try updating yt-dlp", "Попробовать обновить yt-dlp"),
        loc.tr("Open yt-dlp FAQ in browser", "Открыть FAQ yt-dlp в браузере"),
        loc.tr("Retry download", "Повторить попытку скачивания"),
        loc.tr("Return to menu", "Вернуться в главное меню")
    };

    return Menu::arrowSelect(loc.tr("DOWNLOAD ERROR", "ОШИБКА СКАЧИВАНИЯ"), desc, options, 0);
}

} // namespace mrcli::ui
