#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/math/transform.hpp>
#include <memory>

namespace Gauge {

struct Node {
    std::string name;
    Transform local_transform;
    Transform global_transform;

    std::vector<Ref<Node>> children;
    std::vector<Ref<Component>> components;
    std::weak_ptr<Node> parent;

    bool active = true;
    bool visible = true;

    void AddChild(const Ref<Node>& p_node);
    void Draw();
    void RefreshTransform();
    void RefreshTransform(Transform p_parent_transform);
};

}  // namespace Gauge
