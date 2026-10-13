#include <cassert>
#include <iostream>
#include "src/services/FormatBuilder.hpp"
#include "src/domain/AppConfig.hpp"

void testFormatBuilderAudioOnly() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    AppConfig config;
    config.onlyAudio = true;
    config.audioFormat = "M4A(AAC)";

    std::string fmt = FormatBuilder::buildFormat(config);
    assert(fmt.find("ba[ext=m4a]") != std::string::npos);
    assert(fmt.find("bestaudio") != std::string::npos);
    std::cout << "[PASS] testFormatBuilderAudioOnly\n";
}

void testFormatBuilderVideoOnly() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    AppConfig config;
    config.onlyVideo = true;
    config.videoResolution = "1080";
    config.videoFps = "60";
    config.videoFormat = "MP4(H.264)";

    std::string fmt = FormatBuilder::buildFormat(config);
    assert(fmt.find("height=1080") != std::string::npos);
    assert(fmt.find("[fps<=?60]") != std::string::npos);
    assert(fmt.find("[vcodec^=avc1]") != std::string::npos);
    assert(fmt.find("bestvideo") != std::string::npos);
    std::cout << "[PASS] testFormatBuilderVideoOnly\n";
}

void testFormatBuilderCombined() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    AppConfig config;
    config.onlyAudio = false;
    config.onlyVideo = false;
    config.videoResolution = "720";
    config.videoFps = "30";

    std::string fmt = FormatBuilder::buildFormat(config);
    assert(fmt.find("height=720") != std::string::npos);
    assert(fmt.find("[fps<=?30]") != std::string::npos);
    assert(fmt.find("+") != std::string::npos);
    std::cout << "[PASS] testFormatBuilderCombined\n";
}

int main() {
    std::cout << "Running FormatBuilder tests...\n";
    testFormatBuilderAudioOnly();
    testFormatBuilderVideoOnly();
    testFormatBuilderCombined();
    std::cout << "All FormatBuilder tests passed successfully!\n";
    return 0;
}
