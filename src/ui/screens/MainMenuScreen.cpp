#include "MainMenuScreen.hpp"
#include "SettingsScreen.hpp"
#include "DownloadScreen.hpp"
#include "src/ui/InputReader.hpp"
#include "src/core/Console.hpp"
#include "src/core/Keyboard.hpp"
#include <iostream>

namespace mrcli::ui::screens {

MainMenuScreen::MainMenuScreen(app::AppContext& ctx) : m_ctx(ctx) {}

char MainMenuScreen::renderMenu() {
    auto& loc = *m_ctx.loc;
    auto deps = m_ctx.dependencyManager->getStatus();

    std::vector<std::string> opts = {
        "--- " + loc.tr("OPERATIONS", "ОПЕРАЦИИ") + " ---",
        " 1. " + loc.tr("Start download", "Начать скачивание"),
        "--- " + loc.tr("PROGRAM", "ПРОГРАММА") + " ---",
        " s. " + loc.tr("Settings", "Настройки"),
        " l. " + std::string(m_ctx.config.language == domain::Language::English ? "Language: English" : "Язык: Русский"),
        " 0. " + loc.tr("Exit (ESC)", "Выход (ESC)"),
    };
    std::vector<int> actions = { -1, '1', -1, 's', 'l', '0' };

    int selected = 1;
    while (true) {
        core::Console::clear();
        core::Console::print("========================================", core::ConsoleColor::Cyan);
        core::Console::print(loc.tr(" MR CLI FOR YT-DLP v1.1.4", " MR CLI ДЛЯ YT-DLP v1.1.4"), core::ConsoleColor::Cyan);
        core::Console::print("========================================", core::ConsoleColor::Cyan);
        core::Console::print("========================================", core::ConsoleColor::Green);
        core::Console::print(" YT-DLP:  " + std::string(deps.ytDlpInstalled ? loc.tr("[OK] installed", "[OK] установлен") : loc.tr("[ERROR] not found", "[ОШИБКА] не найден")),
                             deps.ytDlpInstalled ? core::ConsoleColor::Green : core::ConsoleColor::Red);
        core::Console::print(" FFMPEG:  " + std::string(deps.ffmpegInstalled ? loc.tr("[OK] installed", "[OK] установлен") : loc.tr("[WARNING] not installed", "[ПРЕДУПРЕЖДЕНИЕ] не установлен")),
                             deps.ffmpegInstalled ? core::ConsoleColor::Green : core::ConsoleColor::Yellow);
        core::Console::print(" QuickJS: " + std::string(deps.quickJsInstalled ? loc.tr("[OK] installed", "[OK] установлен") : loc.tr("[WARNING] not installed", "[ПРЕДУПРЕЖДЕНИЕ] не установлен")),
                             deps.quickJsInstalled ? core::ConsoleColor::Green : core::ConsoleColor::Yellow);
        core::Console::print("========================================", core::ConsoleColor::Green);
        std::cout << "========================================\n";

        for (int i = 0; i < static_cast<int>(opts.size()); ++i) {
            if (actions[i] == -1) {
                std::cout << opts[i] << "\n";
            } else if (i == selected) {
                core::Console::setColor(core::ConsoleColor::Green);
                std::cout << " > " << opts[i] << "\n";
                core::Console::setColor(core::ConsoleColor::White);
            } else {
                std::cout << "   " << opts[i] << "\n";
            }
        }
        std::cout << "========================================\n";
        std::cout << "\n" << loc.tr("Arrow keys to select, Enter to confirm, ESC or 0 to exit\n",
                                    "Стрелки для выбора, Enter для подтверждения, ESC или 0 для выхода\n");

        wint_t key = core::Keyboard::readWideChar();
        if (key == 27 || key == '0') return '0';
        if (key == 13) return static_cast<char>(actions[selected]);

        if (key == 0 || key == 0xE0) {
            wint_t scan = core::Keyboard::readWideChar();
            if (scan == 72) {
                do {
                    selected = (selected > 0) ? selected - 1 : static_cast<int>(opts.size()) - 1;
                } while (actions[selected] == -1);
            } else if (scan == 80) {
                do {
                    selected = (selected < static_cast<int>(opts.size()) - 1) ? selected + 1 : 0;
                } while (actions[selected] == -1);
            }
        } else {
            char ch = core::Keyboard::normalizeKeyToEnglish(key);
            for (int i = 0; i < static_cast<int>(opts.size()); ++i) {
                if (actions[i] == -1) continue;
                if (opts[i].length() >= 2 && opts[i][0] == ' ' && opts[i][1] == ch) {
                    return static_cast<char>(actions[i]);
                }
                if (opts[i].length() >= 1 && opts[i][0] == ch) {
                    return static_cast<char>(actions[i]);
                }
            }
        }
    }
}

void MainMenuScreen::show() {
    while (true) {
        char ch = renderMenu();
        if (ch == 27 || ch == '0') {
            std::cout << m_ctx.loc->tr("Exiting...\n", "Выход...\n");
            return;
        }

        switch (ch) {
        case '1': {
            DownloadScreen ds(m_ctx);
            ds.show();
            break;
        }
        case 's':
        case '2': {
            SettingsScreen ss(m_ctx);
            ss.show();
            break;
        }
        case 'l': {
            m_ctx.config.language = (m_ctx.config.language == domain::Language::English) ?
                domain::Language::Russian : domain::Language::English;
            m_ctx.loc->setLanguage(m_ctx.config.language);
            m_ctx.saveConfig();
            break;
        }
        default: {
            m_ctx.logger->error(m_ctx.loc->tr("[ERROR] Invalid choice!", "[ОШИБКА] Неверный выбор!"));
            InputReader::waitForKey();
            break;
        }
        }
    }
}

} // namespace mrcli::ui::screens
