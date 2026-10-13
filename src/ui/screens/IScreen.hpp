#pragma once

namespace mrcli::ui::screens {

class IScreen {
public:
    virtual ~IScreen() = default;
    virtual void show() = 0;
};

} // namespace mrcli::ui::screens
