#pragma once
#include "ILogger.hpp"
#include <mutex>

namespace mrcli::services {

class ConsoleLogger : public ILogger {
public:
    void log(LogLevel level, std::string_view message) override;
    void info(std::string_view message) override;
    void warn(std::string_view message) override;
    void error(std::string_view message) override;
    void success(std::string_view message) override;

private:
    std::mutex m_mutex;
};

} // namespace mrcli::services
