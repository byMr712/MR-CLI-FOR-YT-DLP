#pragma once

#include "AppContext.hpp"

namespace mrcli::app {

class Application {
public:
    Application();
    ~Application();

    int run();

private:
    bool initialize();
    bool checkDependencies();

    AppContext m_ctx;
};

} // namespace mrcli::app
