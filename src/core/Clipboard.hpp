#pragma once
#include <string>

namespace mrcli::core {

class Clipboard {
public:
    [[nodiscard]] static std::string getText();
    static bool setText(const std::string& text);
};

} // namespace mrcli::core
