#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include "FileSystem.hpp"

namespace mrcli::core {

class ZipExtractor {
public:
    using ProgressCallback = std::function<void(double percent, const std::string& info)>;

    static bool extract(
        const PathArg& zipFilePath,
        const PathArg& destinationDirectory,
        ProgressCallback onProgress = nullptr
    );

    static bool extractZip(
        const PathArg& zipFilePath,
        const PathArg& destinationDirectory,
        ProgressCallback onProgress = nullptr
    ) {
        return extract(zipFilePath, destinationDirectory, onProgress);
    }
};

} // namespace mrcli::core
