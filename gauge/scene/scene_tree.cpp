#include "scene_tree.hpp"

using namespace Gauge;

void SceneTree::Draw() {
    root->Draw();
}

static bool NodeProcessEvent(Ref<Node> node, const SDL_Event& event) {
    if (!node->ProcessEvent(event)) {
        for (auto child : node->children) {
            if (NodeProcessEvent(child, event)) {
                return true;
            }
        }
        return false;
    } else {
        return true;
    }
}

bool SceneTree::ProcessEvent(const SDL_Event& event) {
    return NodeProcessEvent(root, event);
}