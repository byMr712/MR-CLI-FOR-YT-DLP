#pragma once

#include <memory>
#include "src/domain/AppConfig.hpp"
#include "src/domain/DependencyInfo.hpp"
#include "src/services/PathProvider.hpp"
#include "src/services/ILogger.hpp"
#include "src/services/LocalizationService.hpp"
#include "src/services/ConfigService.hpp"
#include "src/services/ArchiveService.hpp"
#include "src/services/CookieService.hpp"
#include "src/services/DependencyManager.hpp"
#include "src/services/YtDlpService.hpp"

namespace mrcli::app {

struct AppContext {
    std::shared_ptr<services::PathProvider> paths;
    std::shared_ptr<services::ILogger> logger;
    std::shared_ptr<services::LocalizationService> loc;
    std::shared_ptr<services::ConfigService> configService;
    std::shared_ptr<services::ArchiveService> archiveService;
    std::shared_ptr<services::CookieService> cookieService;
    std::shared_ptr<services::DependencyManager> dependencyManager;
    std::shared_ptr<services::YtDlpService> ytDlpService;
    domain::AppConfig config;

    void saveConfig() {
        if (configService) {
            configService->save(config);
        }
    }
};

} // namespace mrcli::app
