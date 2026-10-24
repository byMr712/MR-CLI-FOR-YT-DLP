#include "Menu.hpp"
#include "src/core/Console.hpp"
#include "src/core/Keyboard.hpp"
#include "src/services/LocalizationService.hpp"
#include <iostream>

namespace mrcli::ui {

static bool isBackOption(std::string_view opt, bool isLastItem) noexcept {
    if (!isLastItem) return false;
    return (opt.find("Назад") != std::string_view::npos ||
            opt.find("Back") != std::string_view::npos ||
            opt.find("Вернуться") != std::string_view::npos ||
            opt.find("Return") != std::string_view::npos);
}

static char getOptionHotkey(std::string_view opt, int index, size_t totalOptions) noexcept {
    bool isLast = (index == static_cast<int>(totalOptions) - 1);
    if (isBackOption(opt, isLast)) {
        return '0';
    }
    if (opt.size() >= 3 && opt[1] == '.' && opt[2] == ' ') {
        char c = opt[0];
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
        return c;
    }
    if (index < 9) {
        return static_cast<char>('1' + index);
    }
    return static_cast<char>('a' + (index - 9));
}

static std::string formatOptionLine(std::string_view opt, int index, size_t totalOptions) {
    bool isLast = (index == static_cast<int>(totalOptions) - 1);
    if (isBackOption(opt, isLast)) {
        if (opt.size() >= 3 && opt[0] == '0' && opt[1] == '.' && opt[2] == ' ') {
            return std::string(opt);
        }
        return "0. " + std::string(opt);
    }
    if (opt.size() >= 3 && opt[1] == '.' && opt[2] == ' ') {
        return std::string(opt);
    }
    if (index < 9) {
        return std::to_string(index + 1) + ". " + std::string(opt);
    }
    char letter = static_cast<char>('a' + (index - 9));
    return std::string(1, letter) + ". " + std::string(opt);
}

int Menu::arrowSelect(const std::string& title,
                      const std::string& description,
                      const std::vector<std::string>& options,
                      int defaultSelection,
                      const std::vector<std::string>& hints) {
    if (options.empty()) return -1;

    int selected = (defaultSelection >= 0 && defaultSelection < static_cast<int>(options.size())) ? defaultSelection : 0;

    core::Console::clear();
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD home = { 0, 0 };

    while (true) {
        SetConsoleCursorPosition(hOut, home);
        core::Console::print("========================================", core::ConsoleColor::Cyan);
        core::Console::print(" " + title, core::ConsoleColor::Cyan);
        core::Console::print("========================================", core::ConsoleColor::Cyan);

        if (!description.empty()) {
            std::cout << description << "\n\n";
        }

        std::cout << "----------------------------------------\n";
        for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            std::string lineText = formatOptionLine(options[i], i, options.size());

            if (i == selected) {
                core::Console::setColor(core::ConsoleColor::Green);
                std::cout << " > " << lineText << "\033[K\n";
                core::Console::setColor(core::ConsoleColor::White);
            } else {
                std::cout << "   " << lineText << "\033[K\n";
            }
        }
        std::cout << "----------------------------------------\n";

        if (!hints.empty()) {
            std::string hintStr = (selected >= 0 && selected < static_cast<int>(hints.size()) && !hints[selected].empty())
                ? ("[?] " + hints[selected]) : "";
            core::Console::print(hintStr + "\033[K", core::ConsoleColor::DarkGray);
            std::cout << "\n";
        }

        std::string instruction = s_loc ? s_loc->tr("Arrows to select, Enter to confirm, ESC to return",
                                                    "Стрелки для выбора, Enter для подтверждения, ESC для возврата")
                                        : "Arrows to select, Enter to confirm, ESC to return";
        core::Console::print(instruction + "\033[K", core::ConsoleColor::Yellow);

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
            for (int i = 0; i < static_cast<int>(options.size()); ++i) {
                if (getOptionHotkey(options[i], i, options.size()) == ch) {
                    return i;
                }
            }
        }
    }
}

int Menu::inlineMenuSelect(const std::vector<std::string>& options,
                           const std::string& hint1,
                           const std::string& hint2,
                           int escReturnValue,
                           bool clearOnExit) {
    if (options.empty()) return 0;

    std::string countStr = std::to_string(options.size());
    std::string h1 = !hint1.empty() ? hint1 :
        (s_loc ? s_loc->tr(" Arrows to select, Enter to confirm, ESC to return",
                           " Стрелки для выбора, Enter для подтверждения, ESC для возврата")
               : " Arrows to select, Enter to confirm, ESC to return");
    std::string h2 = hint2;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);

    int linesNeeded = static_cast<int>(options.size()) + (h2.empty() ? 5 : 6);
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
                std::cout << " > " << (i + 1) << ". " << options[i] << "\033[K\n";
                core::Console::setColor(core::ConsoleColor::White);
            } else {
                std::cout << "   " << (i + 1) << ". " << options[i] << "\033[K\n";
            }
        }
        core::Console::print("--------------------------------------------", core::ConsoleColor::Cyan);
        core::Console::setColor(core::ConsoleColor::Yellow);
        std::cout << h1 << "\033[K\n";
        if (!h2.empty()) {
            std::cout << h2 << "\033[K\n";
        }
        core::Console::setColor(core::ConsoleColor::White);

        wint_t key = core::Keyboard::readWideChar();
        if (key == 13) {
            if (clearOnExit) {
                SetConsoleCursorPosition(hOut, startCoord);
                std::string emptyLine(80, ' ');
                for (int k = 0; k < linesNeeded; ++k) {
                    std::cout << emptyLine << "\n";
                }
                SetConsoleCursorPosition(hOut, startCoord);
            }
            return selected;
        }
        if (key == 27) { // ESC
            while (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { Sleep(10); }
            FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
            if (clearOnExit) {
                SetConsoleCursorPosition(hOut, startCoord);
                std::string emptyLine(80, ' ');
                for (int k = 0; k < linesNeeded; ++k) {
                    std::cout << emptyLine << "\n";
                }
                SetConsoleCursorPosition(hOut, startCoord);
            }
            return escReturnValue;
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
                if (clearOnExit) {
                    SetConsoleCursorPosition(hOut, startCoord);
                    std::string emptyLine(80, ' ');
                    for (int k = 0; k < linesNeeded; ++k) {
                        std::cout << emptyLine << "\n";
                    }
                    SetConsoleCursorPosition(hOut, startCoord);
                }
                return (ch - '1');
            }
            if (ch == 'q' || ch == 'Q') {
                return (options.size() > 2) ? 2 : 1;
            }
        }
    }
}

} // namespace mrcli::ui
