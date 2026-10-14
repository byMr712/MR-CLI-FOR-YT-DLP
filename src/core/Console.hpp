#pragma once
#include "WinLibs.hpp"
#include <string>
#include <string_view>
#include <iostream>

namespace mrcli::core {

enum class ConsoleColor : WORD {
    Black = 0,
    Blue = 1,
    Green = 2,
    Red = 4,
    Yellow = 6,
    White = 7,
    DarkGray = 8,
    Cyan = 11
};

class Console {
public:
    static void init() noexcept;
    static void initUtf8() noexcept { init(); }
    static void setColor(ConsoleColor color) noexcept;
    static void resetColor() noexcept;
    static void clearScreen() noexcept;
    static void clear() noexcept { clearScreen(); }

    static void printColored(std::string_view text, ConsoleColor color, bool newline = true);
    static void print(std::string_view text, ConsoleColor color = ConsoleColor::White, bool newline = true) {
        printColored(text, color, newline);
    }
    
    [[nodiscard]] static COORD getCursorPosition() noexcept;
    static void setCursorPosition(COORD pos) noexcept;
    static void ensureVerticalSpace(int linesNeeded) noexcept;
};

} // namespace mrcli::core
