#pragma once
#include "Types.hpp"
#include <string>

namespace mrcli::domain {

struct AppConfig {
    std::string downloadPath;
    std::string cookiesFile{"cookies.txt"};
    bool useCookies{true};

    std::string videoResolution{"1080"};
    std::string videoFps{"60"};
    std::string videoFormat{"MP4(H.264)"};
    std::string audioFormat{"M4A(AAC)"};

    bool codecRecompiler{false};
    bool useArchive{true};
    bool onlyAudio{false};
    bool onlyVideo{false};

    bool playlistDelayEnabled{true};
    int playlistDelayInterval{10};
    int playlistDelayDuration{30};

    Language language{Language::English};
};

} // namespace mrcli::domain
