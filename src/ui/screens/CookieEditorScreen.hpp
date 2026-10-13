#pragma once

#include "IScreen.hpp"
#include "src/app/AppContext.hpp"

namespace mrcli::ui::screens {

enum class CookieEditorResult {
    Exit,
    Continue,
    Settings
};

class CookieEditorScreen : public IScreen {
public:
    explicit CookieEditorScreen(app::AppContext& ctx, bool fromSettings = false);
    void show() override;
    CookieEditorResult run();

private:
    app::AppContext& m_ctx;
    bool m_fromSettings;
};

} // namespace mrcli::ui::screens
