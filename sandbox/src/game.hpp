#include <gauge.hpp>

using namespace Gauge;

namespace Sandbox {
    struct Game : public App {
        Window window;
        int run() override;
        void update() override;
    };
}