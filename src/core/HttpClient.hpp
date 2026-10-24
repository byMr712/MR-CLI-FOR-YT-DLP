#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include <functional>
#include <cstdint>
#include "FileSystem.hpp"

namespace mrcli::core {

using ProgressCallback = std::function<void(uint64_t downloadedBytes, uint64_t totalBytes)>;

class HttpClient {
public:
    static bool downloadFile(
        std::string_view url,
        const PathArg& destinationPath,
        ProgressCallback onProgress = nullptr
    );
};

} // namespace mrcli::core
