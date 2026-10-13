#pragma once

#include <string>
#include <memory>
#include "src/domain/DependencyInfo.hpp"
#include "src/services/PathProvider.hpp"
#include "src/services/ILogger.hpp"
#include "src/services/LocalizationService.hpp"

namespace mrcli::services {

class DependencyManager {
public:
    DependencyManager(std::shared_ptr<PathProvider> paths,
                      std::shared_ptr<ILogger> logger,
                      std::shared_ptr<LocalizationService> loc);

    void refreshStatus();
    const domain::DependencyStatus& getStatus() const;

    bool checkAndCopyFromPeerFFmpeg();
    bool downloadAndExtractTool(const std::string& name,
                               const std::string& url,
                               const std::string& targetExe,
                               const std::string& label,
                               bool isZip);

    bool installYtDlp();
    bool installFFmpeg();
    bool installQuickJs();

    bool ensureAllDependencies(bool allowAutoInstall);

private:
    void organizeExtractedTool(const std::string& targetExe, const std::string& destDir);

    std::shared_ptr<PathProvider> m_paths;
    std::shared_ptr<ILogger> m_logger;
    std::shared_ptr<LocalizationService> m_loc;
    domain::DependencyStatus m_status;
};

} // namespace mrcli::services
