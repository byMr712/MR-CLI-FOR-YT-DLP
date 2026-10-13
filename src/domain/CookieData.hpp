#pragma once
#include <string>
#include <vector>

namespace mrcli::domain {

struct CookieEntry {
    std::string domain;
    bool includeSubdomains{false};
    std::string path;
    bool secure{false};
    int64_t expires{0};
    std::string name;
    std::string value;
};

struct CookieStoreInfo {
    bool hasEncryptedDat{false};
    bool hasPlainTxt{false};
    size_t validLinesCount{0};
};

} // namespace mrcli::domain
