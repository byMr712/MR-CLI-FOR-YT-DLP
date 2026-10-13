#pragma once

#include <string>
#include <vector>
#include "src/domain/Types.hpp"
#include "src/services/LocalizationService.hpp"

namespace mrcli::ui {

class Dialogs {
public:
    static std::string openFolderDialog(const std::wstring& title = L"Select folder");
    static std::string openFileDialog(bool isRussian = false);

    static int showPoTokenRecoveryMenu(bool isPlaylist, const std::string& pluginsPath, services::LocalizationService& loc);
    static int showAuthRequiredMenu(bool isPlaylist, bool isAge, services::LocalizationService& loc);
    static int showDrmErrorMenu(bool isPlaylist, services::LocalizationService& loc);
    static int showUnavailableMenu(bool isPlaylist, services::LocalizationService& loc);
    static int showFormatErrorMenu(bool isPlaylist, services::LocalizationService& loc);
    static int showNetworkErrorMenu(bool isPlaylist, services::LocalizationService& loc);
    static int showSingleVideoErrorMenu(services::LocalizationService& loc);
};

} // namespace mrcli::ui
