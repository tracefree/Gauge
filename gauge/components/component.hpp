#pragma once

#include <gauge/common.hpp>
#include <string>

namespace Gauge {

struct Node;

struct Component {
    std::string name;
    bool active = true;
    bool visible = true;
    Ref<Node> node;

    virtual void Update() {}
    virtual void Draw() {}
};

}  // namespace Gauge