#pragma once
#include <string_view>

namespace mrcli::services {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Success
};

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(LogLevel level, std::string_view message) = 0;
    virtual void info(std::string_view message) = 0;
    virtual void warn(std::string_view message) = 0;
    virtual void error(std::string_view message) = 0;
    virtual void success(std::string_view message) = 0;
};

} // namespace mrcli::services
