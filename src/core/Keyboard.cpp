#include "Keyboard.hpp"
#include <iostream>

namespace mrcli::core {

char Keyboard::normalizeKeyToEnglish(wint_t wc) noexcept {
    if (wc < 128) return static_cast<char>(wc);
    switch (wc) {
        case 0x0439: case 0x0419: return 'q';
        case 0x0446: case 0x0426: return 'w';
        case 0x0443: case 0x0423: return 'e';
        case 0x043A: case 0x041A: return 'r';
        case 0x0435: case 0x0415: return 't';
        case 0x043D: case 0x041D: return 'y';
        case 0x0433: case 0x0413: return 'u';
        case 0x0448: case 0x0428: return 'i';
        case 0x0449: case 0x0429: return 'o';
        case 0x0437: case 0x0417: return 'p';
        case 0x0445: case 0x0425: return '[';
        case 0x044A: case 0x042A: return ']';
        case 0x0444: case 0x0424: return 'a';
        case 0x044B: case 0x042B: return 's';
        case 0x0432: case 0x0412: return 'd';
        case 0x0430: case 0x0410: return 'f';
        case 0x043F: case 0x041F: return 'g';
        case 0x0440: case 0x0420: return 'h';
        case 0x043E: case 0x041E: return 'j';
        case 0x043B: case 0x041B: return 'k';
        case 0x0434: case 0x0414: return 'l';
        case 0x0436: case 0x0416: return ';';
        case 0x044D: case 0x042D: return '\'';
        case 0x044F: case 0x042F: return 'z';
        case 0x0447: case 0x0427: return 'x';
        case 0x0441: case 0x0421: return 'c';
        case 0x043C: case 0x041C: return 'v';
        case 0x0438: case 0x0418: return 'b';
        case 0x0442: case 0x0422: return 'n';
        case 0x044C: case 0x042C: return 'm';
        case 0x0431: case 0x0411: return ',';
        case 0x044E: case 0x042E: return '.';
        default: return static_cast<char>(wc & 0xFF);
    }
}

bool Keyboard::hasKey() noexcept {
    return _kbhit() != 0;
}

wint_t Keyboard::readWideKey() noexcept {
    return _getwch();
}

int Keyboard::readChar() noexcept {
    return _getch();
}

void Keyboard::waitForKey() noexcept {
    (void)_getch();
}

} // namespace mrcli::core
