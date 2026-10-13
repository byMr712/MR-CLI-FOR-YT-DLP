#include <cassert>
#include <iostream>
#include "src/services/LocalizationService.hpp"

void testLocalizationSwitch() {
    using namespace mrcli::services;
    using namespace mrcli::domain;

    LocalizationService loc;
    loc.setLanguage(Language::English);
    assert(loc.get(StrId::AppTitle) == "MR CLI FOR YT DLP v1.1.3");
    assert(loc.tr("Start download", "Начать скачивание") == "Start download");

    loc.setLanguage(Language::Russian);
    assert(loc.get(StrId::AppTitle) == "MR CLI FOR YT DLP v1.1.3");
    assert(loc.tr("Start download", "Начать скачивание") == "Начать скачивание");

    std::cout << "[PASS] testLocalizationSwitch\n";
}

int main() {
    std::cout << "Running Localization tests...\n";
    testLocalizationSwitch();
    std::cout << "All Localization tests passed successfully!\n";
    return 0;
}
