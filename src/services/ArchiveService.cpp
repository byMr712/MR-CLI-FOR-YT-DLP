#include "ArchiveService.hpp"
#include "../core/FileSystem.hpp"
#include "../core/StringUtil.hpp"
#include <fstream>
#include <vector>

namespace mrcli::services {

ArchiveService::ArchiveService(std::filesystem::path archiveFilePath)
    : m_archivePath(std::move(archiveFilePath)) {}

bool ArchiveService::exists() const noexcept {
    return core::FileSystem::fileExists(m_archivePath);
}

bool ArchiveService::clear() noexcept {
    if (exists()) {
        return core::FileSystem::removeFile(m_archivePath);
    }
    return true;
}

bool ArchiveService::removeId(const std::string& videoId) {
    if (videoId.empty() || !exists()) return false;

    std::ifstream inFile(m_archivePath);
    if (!inFile.is_open()) return false;

    std::vector<std::string> lines;
    std::string line;
    bool found = false;

    while (std::getline(inFile, line)) {
        if (!line.empty() && line.find(videoId) != std::string::npos) {
            found = true;
            continue;
        }
        lines.push_back(line);
    }
    inFile.close();

    if (!found) return false;

    std::ofstream outFile(m_archivePath, std::ios::trunc);
    if (!outFile.is_open()) return false;

    for (const auto& l : lines) {
        outFile << l << "\n";
    }
    outFile.close();
    return true;
}

} // namespace mrcli::services
