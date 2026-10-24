#pragma once
#include <filesystem>
#include "FileSystem.hpp"

namespace mrcli::core {

class ZipExtractor {
public:
    static bool extract(
        const PathArg& zipFilePath,
        const PathArg& destinationDirectory
    );

    static bool extractZip(
        const PathArg& zipFilePath,
        const PathArg& destinationDirectory
    ) {
        return extract(zipFilePath, destinationDirectory);
    }
};

} // namespace mrcli::core
