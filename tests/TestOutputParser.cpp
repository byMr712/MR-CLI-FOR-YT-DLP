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

void testOutputParserMergerQuotes() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "[Merger] Merging formats into \"C:\\downloads\\video.mp4\"";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.event == LineEvent::Merger);
    assert(result.mediaName == "C:\\downloads\\video.mp4");
    assert(result.destinationPath == "C:\\downloads\\video.mp4");
    std::cout << "[PASS] testOutputParserMergerQuotes\n";
}

void testOutputParserAlreadyInArchive() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "[download] 007 - Monetochka - Zaporozhets: has already been recorded in the archive";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.event == LineEvent::AlreadyInArchive);
    assert(result.mediaName == "007 - Monetochka - Zaporozhets");
    std::cout << "[PASS] testOutputParserAlreadyInArchive\n";
}

void testOutputParserAlreadyDownloaded() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    std::string line = "[download] C:\\Users\\Mr712\\Documents\\MR-CLI-FOR-YT-DLP\\downloads\\[Playlist] monetochka\\003 - [Monetochka] Nimfomanka.mp4 has already been downloaded";
    auto result = OutputParser::parseLine(line, false, false);

    assert(result.event == LineEvent::AlreadyInArchive);
    assert(result.mediaName == "003 - [Monetochka] Nimfomanka");
    assert(result.destinationPath == "C:\\Users\\Mr712\\Documents\\MR-CLI-FOR-YT-DLP\\downloads\\[Playlist] monetochka\\003 - [Monetochka] Nimfomanka.mp4");
    std::cout << "[PASS] testOutputParserAlreadyDownloaded\n";
}

int main() {
    std::cout << "Running OutputParser tests...\n";
    testOutputParserProgress();
    testOutputParserItemTransition();
    testOutputParserPoTokenError();
    testOutputParserDestinationCleanup();
    testOutputParserMergerQuotes();
    testOutputParserAlreadyInArchive();
    testOutputParserAlreadyDownloaded();
    std::cout << "All OutputParser tests passed successfully!\n";
    return 0;
}
