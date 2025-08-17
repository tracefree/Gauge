#include "node.hpp"
#include "gauge/components/component.hpp"

using namespace Gauge;

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