#include "ProgressRenderer.hpp"
#include "src/core/Console.hpp"
#include "src/core/Keyboard.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace mrcli::ui {

void ProgressRenderer::renderProgressBar(const domain::ProgressInfo& info, const std::string& etaLabel) {
    double percent = info.percent;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    const int barWidth = 30;
    int pos = static_cast<int>(barWidth * percent / 100.0);

    std::cout << "\r[";
    for (int i = 0; i < barWidth; ++i) {
        std::cout << (i < pos ? '=' : (i == pos ? '>' : ' '));
    }
    std::cout << "] ";

    std::cout << std::fixed << std::setprecision(1);
    if (percent < 10.0) std::cout << "  ";
    else if (percent < 100.0) std::cout << " ";
    std::cout << percent << "%";

    if (!info.speed.empty()) {
        std::cout << "  " << info.speed;
    }
    if (!info.eta.empty()) {
        std::cout << "  " << etaLabel << info.eta << "        ";
    } else {
        std::cout << "                        ";
    }
    std::cout << std::flush;
}

void ProgressRenderer::renderComponentProgress(const std::string& label, double percent, const std::string& extraInfo) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    const int barWidth = 25;
    int pos = static_cast<int>(barWidth * percent / 100.0);

    std::cout << "\r";
    if (!label.empty()) {
        std::cout << "[" << label << "] ";
    }
    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        std::cout << (i < pos ? '=' : (i == pos ? '>' : ' '));
    }
    std::cout << "] ";

    std::cout << std::setw(3) << static_cast<int>(percent) << "%";

    if (!extraInfo.empty()) {
        std::cout << " " << extraInfo;
    }
    std::cout << "        " << std::flush;
}

bool ProgressRenderer::renderDelayCountdown(int completedCount, int durationSeconds,
                                           const std::function<std::string(const std::string&, const std::string&)>& tr) {
    if (durationSeconds <= 0) return true;

    core::Console::print("----------------------------------------------------------------------", core::ConsoleColor::Cyan);
    core::Console::setColor(core::ConsoleColor::Yellow);
    std::cout << " [!] " << tr("Playlist delay: ", "Задержка для плейлиста: ")
              << completedCount << tr(" videos downloaded.", " видео скачано.") << "\n";
    std::cout << "     " << tr("Pausing to prevent YouTube rate limits (HTTP 429).",
                              "Пауза для защиты от блокировок YouTube (HTTP 429).") << "\n";
    std::cout << "     " << tr("Press ESC, Enter or Space to skip delay, Q to stop download.",
                              "Нажмите ESC, Enter или Пробел для пропуска, Q для отмены загрузки.") << "\n";
    core::Console::setColor(core::ConsoleColor::White);
    core::Console::print("----------------------------------------------------------------------", core::ConsoleColor::Cyan);

    bool cancelled = false;
    bool skipped = false;

    for (int sec = durationSeconds; sec > 0; --sec) {
        core::Console::setColor(core::ConsoleColor::Cyan);
        std::cout << "\r" << tr(" [PAUSE] Resuming in ", " [ПАУЗА] Продолжение через ")
                  << sec << tr("s... (ESC: skip, Q: cancel)   ", "с... (ESC: пропуск, Q: отмена)   ") << std::flush;
        core::Console::setColor(core::ConsoleColor::White);

        bool skip = false;
        for (int ms = 0; ms < 10; ++ms) {
            if (core::Keyboard::hasKey()) {
                wint_t k = core::Keyboard::readWideChar();
                char c = core::Keyboard::normalizeKeyToEnglish(k);
                if (k == 27 || k == 13 || k == ' ') {
                    skip = true;
                    skipped = true;
                    break;
                } else if (c == 'q' || c == 'Q' || k == 3) {
                    cancelled = true;
                    break;
                }
            }
            Sleep(100);
        }

        if (cancelled) {
            core::Console::setColor(core::ConsoleColor::Red);
            std::cout << "\r" << tr(" [PAUSE] Download cancelled by user!                 \n",
                                   " [ПАУЗА] Загрузка отменена пользователем!             \n") << std::flush;
            core::Console::setColor(core::ConsoleColor::White);
            break;
        }
        if (skip) {
            core::Console::setColor(core::ConsoleColor::Yellow);
            std::cout << "\r" << tr(" [PAUSE] Delay skipped! Resuming download...           \n",
                                   " [ПАУЗА] Пауза пропущена! Возобновление загрузки...    \n") << std::flush;
            core::Console::setColor(core::ConsoleColor::White);
            break;
        }
    }

    if (!cancelled) {
        if (!skipped) {
            std::cout << "\n\n";
        } else {
            std::cout << "\n";
        }
    }
    return !cancelled;
}

} // namespace mrcli::ui
