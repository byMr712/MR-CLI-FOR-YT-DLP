#pragma once

#include "IScreen.hpp"
#include "src/app/AppContext.hpp"

namespace mrcli::ui::screens {

class SettingsScreen : public IScreen {
public:
    explicit SettingsScreen(app::AppContext& ctx);
    void show() override;

private:
    void selectVideoQuality();
    void selectAudioQuality();
    void codecRecompilerMenu();
    void toggleUseArchive();
    void toggleOnlyAudio();
    void toggleOnlyVideo();
    void playlistDelayMenu();
    void clearArchiveMenu();
    void updateComponentsMenu();

    app::AppContext& m_ctx;
};

} // namespace mrcli::ui::screens
