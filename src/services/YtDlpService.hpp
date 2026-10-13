#pragma once

#include <string>
#include <memory>
#include "src/domain/AppConfig.hpp"
#include "src/domain/DownloadOptions.hpp"
#include "src/domain/DependencyInfo.hpp"
#include "src/services/IDownloadObserver.hpp"
#include "src/services/PathProvider.hpp"
#include "src/services/ArchiveService.hpp"
#include "src/services/CookieService.hpp"
#include "src/services/ILogger.hpp"
#include "src/services/LocalizationService.hpp"

namespace mrcli::services {

class YtDlpService {
public:
    YtDlpService(std::shared_ptr<PathProvider> paths,
                 std::shared_ptr<ArchiveService> archive,
                 std::shared_ptr<CookieService> cookies,
                 std::shared_ptr<ILogger> logger,
                 std::shared_ptr<LocalizationService> loc);

    std::wstring buildCommand(const domain::AppConfig& config,
                              const domain::DownloadOptions& options,
                              const domain::DependencyStatus& deps) const;

    domain::DownloadResult execute(const domain::AppConfig& config,
                                  const domain::DownloadOptions& options,
                                  const domain::DependencyStatus& deps,
                                  IDownloadObserver* observer);

private:
    bool manualFfmpegMerge(const std::string& mergeErrorPath, const std::string& ffmpegPath);

    std::shared_ptr<PathProvider> m_paths;
    std::shared_ptr<ArchiveService> m_archive;
    std::shared_ptr<CookieService> m_cookies;
    std::shared_ptr<ILogger> m_logger;
    std::shared_ptr<LocalizationService> m_loc;
};

} // namespace mrcli::services
