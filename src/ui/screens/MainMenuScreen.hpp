#pragma once

#include "IScreen.hpp"
#include "src/app/AppContext.hpp"

namespace mrcli::ui::screens {

class MainMenuScreen : public IScreen {
public:
    explicit MainMenuScreen(app::AppContext& ctx);
    void show() override;

private:
    char renderMenu();

    app::AppContext& m_ctx;
};

} // namespace mrcli::ui::screens
