#include <cassert>
#include <iostream>
#include "src/services/OutputParser.hpp"

void testOutputParserProgress() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "[download]  45.2% of ~  12.34MiB at  3.45MiB/s ETA 00:02";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.event == LineEvent::Progress);
    assert(result.progress.percent > 45.1 && result.progress.percent < 45.3);
    assert(result.progress.speed == "3.45MiB/s");
    assert(result.progress.eta == "00:02");
    std::cout << "[PASS] testOutputParserProgress\n";
}

void testOutputParserItemTransition() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "[download] Downloading item 3 of 10";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.event == LineEvent::ItemTransition);
    assert(result.curItem == 3);
    assert(result.totalItems == 10);
    std::cout << "[PASS] testOutputParserItemTransition\n";
}

void testOutputParserPoTokenError() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "WARNING: [youtube] aqK0FqOQtvY: web_creator client https formats require a GVS PO Token";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.error == ErrorType::PoTokenBotguard);
    assert(result.possibleVideoId == "aqK0FqOQtvY");
    std::cout << "[PASS] testOutputParserPoTokenError\n";
}

void testOutputParserDestinationCleanup() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "[download] Destination: 003 - My Awesome Video.f137.mp4";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.event == LineEvent::Destination);
    assert(result.mediaName == "003 - My Awesome Video");
    assert(!result.isAudio);
    std::cout << "[PASS] testOutputParserDestinationCleanup\n";
}

int main() {
    std::cout << "Running OutputParser tests...\n";
    testOutputParserProgress();
    testOutputParserItemTransition();
    testOutputParserPoTokenError();
    testOutputParserDestinationCleanup();
    std::cout << "All OutputParser tests passed successfully!\n";
    return 0;
}
