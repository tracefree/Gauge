#include "node.hpp"
#include <memory>
#include "gauge/components/component.hpp"

using namespace Gauge;

void Node::AddChild(Ref<Node>& p_node) {
    // TODO: check if node already has parent
    // TODO: Don't create new Ref here
    children.push_back(p_node);
    p_node->parent = std::make_shared<Node>(*this);
}

void Node::Draw() {
    if (!visible) {
        return;
    }
    for (const Ref<Component>& component : components) {
        if (component->visible) {
            component->Draw();
        }
    }
    for (const Ref<Node>& child : children) {
        child->Draw();
    }
}

void Node::RefreshTransform() {
    RefreshTransform(parent.lock() == nullptr ? Transform() : parent.lock()->global_transform);
}

void Node::RefreshTransform(Transform p_parent_transform) {
    // TODO: Why can't p_parent_transform be const?
    global_transform = p_parent_transform * local_transform;

    for (auto child : children) {
        child->RefreshTransform(global_transform);
    }
}