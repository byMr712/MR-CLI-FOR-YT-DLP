#include "InputReader.hpp"
#include "src/core/Clipboard.hpp"
#include "src/core/Keyboard.hpp"
#include "src/core/StringUtil.hpp"
#include <iostream>

namespace mrcli::ui {

bool InputReader::inputLineWithEscape(std::string& outLine, const std::string& prompt) {
    std::cout << prompt << std::flush;
    outLine.clear();

    while (true) {
        wint_t ch = core::Keyboard::readWideChar();
        if (ch == 27) { // ESC
            std::cout << "\n";
            return false;
        }
        if (ch == 13) { // Enter
            std::cout << "\n";
            return true;
        }
        if (ch == 8) { // Backspace
            if (!outLine.empty()) {
                outLine.pop_back();
                std::cout << "\b \b" << std::flush;
            }
            continue;
        }
        if (ch == 22) { // Ctrl+V
            std::string clip = core::Clipboard::getText();
            if (!clip.empty()) {
                for (char c : clip) {
                    if (c == '\r' || c == '\n') continue;
                    outLine += c;
                    std::cout << c;
                }
                std::cout << std::flush;
            }
            continue;
        }

        if (ch >= 32) {
            std::wstring ws(1, static_cast<wchar_t>(ch));
            std::string u8 = core::StringUtil::wstringToUtf8(ws);
            outLine += u8;
            std::cout << u8 << std::flush;
        }
    }
}

void InputReader::waitForKey() {
    (void)core::Keyboard::readWideChar();
}

} // namespace mrcli::ui
