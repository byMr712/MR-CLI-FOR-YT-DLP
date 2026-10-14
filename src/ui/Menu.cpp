#include "Menu.hpp"
#include "src/core/Console.hpp"
#include "src/core/Keyboard.hpp"
#include "src/services/LocalizationService.hpp"
#include <iostream>

namespace mrcli::ui {

int Menu::arrowSelect(const std::string& title,
                      const std::string& description,
                      const std::vector<std::string>& options,
                      int defaultSelection,
                      const std::vector<std::string>& hints) {
    if (options.empty()) return -1;

    int selected = (defaultSelection >= 0 && defaultSelection < static_cast<int>(options.size())) ? defaultSelection : 0;

    while (true) {
        core::Console::clear();
        core::Console::print("========================================", core::ConsoleColor::Cyan);
        core::Console::print(" " + title, core::ConsoleColor::Cyan);
        core::Console::print("========================================", core::ConsoleColor::Cyan);

        if (!description.empty()) {
            std::cout << description << "\n\n";
        }

        std::cout << "----------------------------------------\n";
        for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            if (i == selected) {
                core::Console::setColor(core::ConsoleColor::Green);
                std::cout << " > " << (i + 1) << ". " << options[i] << "\n";
                core::Console::setColor(core::ConsoleColor::White);
            } else {
                std::cout << "   " << (i + 1) << ". " << options[i] << "\n";
            }
        }
        std::cout << "----------------------------------------\n";

        if (selected >= 0 && selected < static_cast<int>(hints.size()) && !hints[selected].empty()) {
            core::Console::print("[?] " + hints[selected], core::ConsoleColor::DarkGray);
            std::cout << "\n";
        }

        core::Console::setColor(core::ConsoleColor::Yellow);
        std::string instruction = s_loc ? s_loc->tr("Arrows to select, Enter to confirm, ESC to return\n",
                                                    "Стрелки для выбора, Enter для подтверждения, ESC для возврата\n")
                                        : "Arrows to select, Enter to confirm, ESC to return\n";
        std::cout << instruction;
        core::Console::setColor(core::ConsoleColor::White);

        wint_t key = core::Keyboard::readWideChar();
        if (key == 27) { // ESC
            return -1;
        }
        if (key == 13) { // Enter
            return selected;
        }

        if (key == 0 || key == 0xE0) {
            wint_t scan = core::Keyboard::readWideChar();
            if (scan == 72) { // Up
                selected = (selected > 0) ? selected - 1 : static_cast<int>(options.size()) - 1;
            } else if (scan == 80) { // Down
                selected = (selected < static_cast<int>(options.size()) - 1) ? selected + 1 : 0;
            }
        } else {
            char ch = core::Keyboard::normalizeKeyToEnglish(key);
            if (ch >= '1' && ch < '1' + static_cast<int>(options.size())) {
                return (ch - '1');
            }
            if (ch == '0') {
                return -1;
            }
        }
    }
}

int Menu::inlineMenuSelect(const std::vector<std::string>& options,
                           const std::string& hint1,
                           const std::string& hint2) {
    if (options.empty()) return 0;

    std::string countStr = std::to_string(options.size());
    std::string h1 = !hint1.empty() ? hint1 :
        (s_loc ? s_loc->tr(" Arrows to select - Enter to confirm",
                           " Стрелочки для выбор - ввод для подтверждения")
               : " Arrows to select - Enter to confirm");
    std::string h2 = !hint2.empty() ? hint2 :
        (s_loc ? s_loc->tr(" Or numbers 1-" + countStr + " to select without Enter",
                           " Либо цифрами 1-" + countStr + " выбор без подтверждения")
               : " Or numbers 1-" + countStr + " to select without Enter");

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);

    int linesNeeded = static_cast<int>(options.size()) + 6;
    if (csbi.dwCursorPosition.Y + linesNeeded >= csbi.dwSize.Y) {
        for (int k = 0; k < linesNeeded; ++k) std::cout << "\n";
        GetConsoleScreenBufferInfo(hOut, &csbi);
        csbi.dwCursorPosition.Y -= static_cast<SHORT>(linesNeeded);
    }
    COORD startCoord = csbi.dwCursorPosition;

    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    int selected = 0;
    while (true) {
        SetConsoleCursorPosition(hOut, startCoord);
        std::cout << "\n";
        core::Console::print("--------------------------------------------", core::ConsoleColor::Cyan);
        for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            if (i == selected) {
                core::Console::setColor(core::ConsoleColor::Green);
                std::cout << " > " << (i + 1) << ". " << options[i] << "                     \n";
                core::Console::setColor(core::ConsoleColor::White);
            } else {
                std::cout << "   " << (i + 1) << ". " << options[i] << "                     \n";
            }
        }
        core::Console::print("--------------------------------------------", core::ConsoleColor::Cyan);
        core::Console::setColor(core::ConsoleColor::Yellow);
        std::cout << h1 << "         \n";
        std::cout << h2 << "         \n";
        core::Console::setColor(core::ConsoleColor::White);

        wint_t key = core::Keyboard::readWideChar();
        if (key == 13) {
            return selected;
        }
        if (key == 27) { // ESC = Resume
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { Sleep(10); }
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
            return 0;
        }
        if (key == 3) { // Ctrl+C = Cancel all
            return (options.size() > 2) ? 2 : 1;
        }

        if (key == 0 || key == 0xE0) {
            wint_t scan = core::Keyboard::readWideChar();
            if (scan == 72) {
                selected = (selected > 0) ? selected - 1 : static_cast<int>(options.size()) - 1;
            } else if (scan == 80) {
                selected = (selected < static_cast<int>(options.size()) - 1) ? selected + 1 : 0;
            }
        } else {
            char ch = core::Keyboard::normalizeKeyToEnglish(key);
            if (ch >= '1' && ch < '1' + static_cast<int>(options.size())) {
                return (ch - '1');
            }
            if (ch == 'q' || ch == 'Q') {
                return (options.size() > 2) ? 2 : 1;
            }
        }
    }
}

} // namespace mrcli::ui
