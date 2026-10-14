#pragma once
#include "WinLibs.hpp"
#include <conio.h>

namespace mrcli::core {

class Keyboard {
public:
    [[nodiscard]] static char normalizeKeyToEnglish(wint_t wc) noexcept;
    [[nodiscard]] static bool hasKey() noexcept;
    [[nodiscard]] static wint_t readWideKey() noexcept;
    [[nodiscard]] static wint_t readWideChar() noexcept { return readWideKey(); }
    [[nodiscard]] static int readChar() noexcept;
    static void waitForKey() noexcept;
};

} // namespace mrcli::core
