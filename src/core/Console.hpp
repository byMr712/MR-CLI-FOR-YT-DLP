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
    Cyan = 11
};

class Console {
public:
    static void init() noexcept;
    static void setColor(ConsoleColor color) noexcept;
    static void resetColor() noexcept;
    static void clearScreen() noexcept;

    static void printColored(std::string_view text, ConsoleColor color, bool newline = true);
    
    [[nodiscard]] static COORD getCursorPosition() noexcept;
    static void setCursorPosition(COORD pos) noexcept;
    static void ensureVerticalSpace(int linesNeeded) noexcept;
};

} // namespace mrcli::core
