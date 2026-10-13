#include <cassert>
#include <iostream>
#include "src/services/ConfigService.hpp"
#include "src/core/FileSystem.hpp"

void testConfigSaveLoad() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string testPath = "test-mr-config.txt";

    ConfigService service(testPath);
    AppConfig original;
    original.videoResolution = "1440";
    original.videoFps = "30";
    original.videoFormat = "WEBM(VP9)";
    original.audioFormat = "Opus";
    original.codecRecompiler = true;
    original.useArchive = true;
    original.onlyAudio = false;
    original.onlyVideo = true;
    original.playlistDelayEnabled = true;
    original.playlistDelayInterval = 5;
    original.playlistDelayDuration = 15;
    original.language = Language::Russian;

    assert(service.save(original));

    AppConfig loaded = service.load();
    assert(loaded.videoResolution == "1440");
    assert(loaded.videoFps == "30");
    assert(loaded.videoFormat == "WEBM(VP9)");
    assert(loaded.audioFormat == "Opus");
    assert(loaded.codecRecompiler == true);
    assert(loaded.useArchive == true);
    assert(loaded.onlyAudio == false);
    assert(loaded.onlyVideo == true);
    assert(loaded.playlistDelayEnabled == true);
    assert(loaded.playlistDelayInterval == 5);
    assert(loaded.playlistDelayDuration == 15);
    assert(loaded.language == Language::Russian);

    mrcli::core::FileSystem::deleteFile(testPath);
    std::cout << "[PASS] testConfigSaveLoad\n";
}

int main() {
    std::cout << "Running Config tests...\n";
    testConfigSaveLoad();
    std::cout << "All Config tests passed successfully!\n";
    return 0;
}
