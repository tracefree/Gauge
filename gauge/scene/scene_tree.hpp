#pragma once

#include <gauge/common.hpp>
#include <gauge/scene/node.hpp>

namespace Gauge {

struct SceneTree {
    Ref<Node> root;

    void Draw();
    bool ProcessEvent(const SDL_Event& event);
};

}  // namespace Gauge
