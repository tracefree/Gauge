#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/math/transform.hpp>

namespace Gauge {

struct Node {
    std::string name;
    Transform local_transform;
    Transform global_transfom;

    std::vector<Ref<Node>> children;
    std::vector<Ref<Component>> components;

    bool active = true;
    bool visible = true;

    void Draw();
};

}  // namespace Gauge
