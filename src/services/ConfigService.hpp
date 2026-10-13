#pragma once
#include "../domain/AppConfig.hpp"
#include <filesystem>

namespace mrcli::services {

class ConfigService {
public:
    explicit ConfigService(std::filesystem::path configFilePath);

    [[nodiscard]] domain::AppConfig load();
    bool save(const domain::AppConfig& config);

private:
    std::filesystem::path m_configFilePath;
};

} // namespace mrcli::services
