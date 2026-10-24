#pragma once

#include <string>
#include <vector>

namespace mrcli::services {
class LocalizationService;
}

namespace mrcli::ui {

class Menu {
public:
    static void setLocalization(const services::LocalizationService* loc) { s_loc = loc; }

    static int arrowSelect(const std::string& title,
                           const std::string& description,
                           const std::vector<std::string>& options,
                           int defaultSelection = 0,
                           const std::vector<std::string>& hints = {});

    static int inlineMenuSelect(const std::vector<std::string>& options,
                                const std::string& hint1 = "",
                                const std::string& hint2 = "",
                                int escReturnValue = 0,
                                bool clearOnExit = false);

private:
    inline static const services::LocalizationService* s_loc = nullptr;
};

} // namespace mrcli::ui
