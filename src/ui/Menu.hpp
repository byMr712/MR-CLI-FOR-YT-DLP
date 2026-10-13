#pragma once

#include <string>
#include <vector>

namespace mrcli::ui {

class Menu {
public:
    static int arrowSelect(const std::string& title,
                           const std::string& description,
                           const std::vector<std::string>& options,
                           int defaultSelection = 0,
                           const std::vector<std::string>& hints = {});

    static int inlineMenuSelect(const std::vector<std::string>& options);
};

} // namespace mrcli::ui
