#include "CookieEditorScreen.hpp"
#include "src/ui/Menu.hpp"
#include "src/ui/Dialogs.hpp"
#include "src/ui/InputReader.hpp"
#include "src/core/Clipboard.hpp"
#include "src/core/Process.hpp"
#include "src/core/StringUtil.hpp"
#include "src/core/FileSystem.hpp"
#include <iostream>

namespace mrcli::ui::screens {

CookieEditorScreen::CookieEditorScreen(app::AppContext& ctx, bool fromSettings)
    : m_ctx(ctx), m_fromSettings(fromSettings) {}

void CookieEditorScreen::show() {
    run();
}

CookieEditorResult CookieEditorScreen::run() {
    auto& loc = *m_ctx.loc;
    auto& cookies = *m_ctx.cookieService;

    while (true) {
        std::string status = loc.tr("Status: ", "Статус: ") +
            (cookies.hasSavedCookies() ? loc.tr("[CONFIGURED]", "[НАСТРОЕНО]") : loc.tr("[NOT CONFIGURED]", "[НЕ НАСТРОЕНО]"));
        std::string desc = "\n" + status + "\n";

        std::vector<std::string> options = {
            loc.tr("Select cookies file (.txt)", "Выбрать файл куки (.txt)"),
            loc.tr("Paste from clipboard", "Вставить из буфера обмена"),
            loc.tr("Edit in Notepad", "Редактировать в Блокноте"),
            loc.tr("Delete saved cookies", "Удалить сохранённые куки")
        };

        std::vector<std::string> hints = {
            loc.tr("Open file picker to select a Netscape-formatted cookies.txt file", "Открыть проводник для выбора файла куки cookies.txt в формате Netscape"),
            loc.tr("Paste Netscape cookie text or JSON cookies directly from clipboard", "Вставить куки напрямую из буфера обмена (в формате Netscape или JSON)"),
            loc.tr("Open cookies file in Notepad to view or edit cookies manually", "Открыть файл куки в Блокноте для ручного редактирования"),
            loc.tr("Remove saved cookies file and reset cookie settings", "Удалить сохранённый файл куки и сбросить настройки")
        };

        int sel = Menu::arrowSelect(loc.tr("COOKIE EDITOR", "РЕДАКТОР КУКИ"), desc, options, 0, hints);
        if (sel < 0) {
            return CookieEditorResult::Exit;
        }

        switch (sel) {
        case 0: {
            std::string path = Dialogs::openFileDialog(m_ctx.config.language == domain::Language::Russian);
            if (path.empty()) {
                m_ctx.logger->warn(loc.tr("[INFO] Selection cancelled", "[ИНФО] Выбор отменён"));
                InputReader::waitForKey();
                break;
            }
            if (cookies.loadFromFile(path)) {
                m_ctx.config.useCookies = true;
                m_ctx.saveConfig();
                m_ctx.logger->info(loc.tr("[OK] Cookies saved successfully!", "[OK] Куки успешно сохранены!"));
                InputReader::waitForKey();
                return m_fromSettings ? CookieEditorResult::Settings : CookieEditorResult::Continue;
            } else {
                m_ctx.logger->error(loc.tr("[ERROR] Invalid cookies file! Netscape format required.", "[ОШИБКА] Неверный файл куки! Требуется формат Netscape."));
                InputReader::waitForKey();
            }
            break;
        }
        case 1: {
            std::string clip = core::Clipboard::getText();
            if (clip.empty()) {
                m_ctx.logger->error(loc.tr("[ERROR] Clipboard is empty!", "[ОШИБКА] Буфер обмена пуст!"));
                InputReader::waitForKey();
                break;
            }
            if (cookies.saveCookies(clip)) {
                m_ctx.config.useCookies = true;
                m_ctx.saveConfig();
                m_ctx.logger->info(loc.tr("[OK] Cookies saved successfully!", "[OK] Куки успешно сохранены!"));
                InputReader::waitForKey();
                return m_fromSettings ? CookieEditorResult::Settings : CookieEditorResult::Continue;
            }
            m_ctx.logger->error(loc.tr("[ERROR] Invalid cookies format in clipboard!", "[ОШИБКА] Неверный формат куки в буфере обмена!"));
            InputReader::waitForKey();
            break;
        }
        case 2: {
            std::string txtPath = cookies.getCookiesPath();
            if (!core::FileSystem::fileExists(txtPath)) {
                cookies.saveCookies("# Netscape HTTP Cookie File\n# Paste cookies here, save, and close Notepad\n\n");
            }
            std::wstring cmd = L"notepad.exe \"" + core::StringUtil::utf8ToWstring(txtPath) + L"\"";
            core::Process::executeWait(cmd);

            if (core::FileSystem::fileExists(txtPath) && cookies.loadFromFile(txtPath)) {
                m_ctx.config.useCookies = true;
                m_ctx.saveConfig();
                m_ctx.logger->info(loc.tr("[OK] Cookies updated successfully!", "[OK] Куки успешно обновлены!"));
                InputReader::waitForKey();
                return m_fromSettings ? CookieEditorResult::Settings : CookieEditorResult::Continue;
            }
            m_ctx.logger->warn(loc.tr("[INFO] No valid cookies saved.", "[ИНФО] Допустимые куки не сохранены."));
            InputReader::waitForKey();
            break;
        }
        case 3: {
            cookies.deleteCookies();
            m_ctx.config.useCookies = false;
            m_ctx.saveConfig();
            m_ctx.logger->info(loc.tr("[OK] Saved cookies removed!", "[OK] Сохранённые куки удалены!"));
            InputReader::waitForKey();
            break;
        }
        default:
            return CookieEditorResult::Exit;
        }
    }
}

} // namespace mrcli::ui::screens
