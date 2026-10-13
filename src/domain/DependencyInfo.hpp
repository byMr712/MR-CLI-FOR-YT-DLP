#pragma once
#include <string>
#include <filesystem>

namespace mrcli::domain {

struct DependencyInfo {
    std::string name;
    std::string executableName;
    std::filesystem::path resolvedPath;
    std::string downloadUrl;
    bool isArchive{false};
    bool isRequired{true};
    bool isFound{false};
};

} // namespace mrcli::domain
