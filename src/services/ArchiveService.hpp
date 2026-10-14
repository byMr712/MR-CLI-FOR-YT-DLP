#pragma once
#include <filesystem>
#include <string>

namespace mrcli::services {

class ArchiveService {
public:
    explicit ArchiveService(std::filesystem::path archiveFilePath);

    [[nodiscard]] std::filesystem::path getPath() const noexcept { return m_archivePath; }
    [[nodiscard]] std::string getArchivePath() const { return m_archivePath.u8string(); }
    [[nodiscard]] bool exists() const noexcept;
    
    bool clear() noexcept;
    bool removeId(const std::string& videoId);

private:
    std::filesystem::path m_archivePath;
};

} // namespace mrcli::services
