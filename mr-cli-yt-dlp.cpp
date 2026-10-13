// ============================================================================
// MR-CLI-FOR-YT-DLP
// Lightweight standalone Windows CLI frontend for yt-dlp, FFmpeg, and QuickJS
//
// Modern C++17 Modular Architecture
// ============================================================================

#include "src/app/Application.hpp"

int main() {
    mrcli::app::Application app;
    return app.run();
}