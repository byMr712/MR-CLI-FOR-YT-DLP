#pragma once
#include "../domain/AppConfig.hpp"
#include <string>

namespace mrcli::services {

class FormatBuilder {
public:
    [[nodiscard]] static std::string buildFormat(const domain::AppConfig& config);
    [[nodiscard]] static std::string buildVideoFilter(const domain::AppConfig& config);
    [[nodiscard]] static std::string buildAudioFilter(const domain::AppConfig& config);
};

} // namespace mrcli::services
