#include "Console.hpp"
#include <iostream>

namespace mrcli::core {

void Console::init() noexcept {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
}

void Console::setColor(ConsoleColor color) noexcept {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hOut, static_cast<WORD>(color));
    }
}

void Console::resetColor() noexcept {
    setColor(ConsoleColor::White);
}

void Console::clearScreen() noexcept {
    std::cout << "\033[2J\033[H" << std::flush;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written = 0;
    COORD home = { 0, 0 };

    FillConsoleOutputCharacterW(hOut, L' ', cellCount, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

void Console::printColored(std::string_view text, ConsoleColor color, bool newline) {
    setColor(color);
    std::cout << text;
    if (newline) {
        std::cout << '\n';
    }
    std::cout << std::flush;
    resetColor();
}

COORD Console::getCursorPosition() noexcept {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.dwCursorPosition;
    }
    return { 0, 0 };
}

void Console::setCursorPosition(COORD pos) noexcept {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        SetConsoleCursorPosition(hOut, pos);
    }
}

void Console::ensureVerticalSpace(int linesNeeded) noexcept {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        if (csbi.dwCursorPosition.Y + linesNeeded >= csbi.dwSize.Y) {
            for (int k = 0; k < linesNeeded; ++k) {
                std::cout << '\n';
            }
            std::cout << std::flush;
        }
    }
}

} // namespace mrcli::core
