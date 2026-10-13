#pragma once

#include <string>

namespace mrcli::ui {

class InputReader {
public:
    static bool inputLineWithEscape(std::string& outLine, const std::string& prompt = "> ");
    static void waitForKey();
};

} // namespace mrcli::ui
