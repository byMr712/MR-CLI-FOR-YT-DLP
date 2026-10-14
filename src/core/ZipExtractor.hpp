#pragma once
#include <filesystem>

namespace mrcli::core {

class ZipExtractor {
public:
    static bool extract(
        const std::filesystem::path& zipFilePath,
        const std::filesystem::path& destinationDirectory
    );

    static bool extractZip(
        const std::filesystem::path& zipFilePath,
        const std::filesystem::path& destinationDirectory
    ) {
        return extract(zipFilePath, destinationDirectory);
    }
};

} // namespace mrcli::core
