#include "node.hpp"
#include "gauge/components/component.hpp"

using namespace Gauge;

void Node::Draw() {
    for (const Ref<Component>& component : components) {
        component->Draw();
    }
    for (const Ref<Node>& child : children) {
        child->Draw();
    }
}