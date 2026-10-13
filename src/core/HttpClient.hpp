#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include <functional>
#include <cstdint>

namespace mrcli::core {

using ProgressCallback = std::function<void(uint64_t downloadedBytes, uint64_t totalBytes)>;

class HttpClient {
public:
    static bool downloadFile(
        std::string_view url,
        const std::filesystem::path& destinationPath,
        ProgressCallback onProgress = nullptr
    );
};

} // namespace mrcli::core
