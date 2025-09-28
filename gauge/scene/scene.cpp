#include "scene.hpp"

#include <gauge/core/resource_manager.hpp>
#include <gauge/core/string_id.hpp>
#include <gauge/scene/node.hpp>
#include <gauge/scene/yaml.hpp>

#include <memory>
#include <string>

using namespace Gauge;

Ref<Node> Scene::Instantiate() {
    if (state == nullptr) {
        return Node::Create("[Invalid Node]");
    }

    return FromData(*state);
}

Ref<Node> Scene::FromData(YAML::Node p_data) {
    auto node = Node::Create();

    if (p_data["name"]) {
        node->name = p_data["name"].as<std::string>();
    }

    if (p_data["position"]) {
        node->SetPosition(p_data["position"].as<Vec3>());
    }

    if (p_data["scale"]) {
        node->ScaleBy(p_data["scale"].as<float>());
    }

    for (const YAML::Node& component_node : p_data["components"]) {
        const auto& type = component_node["type"].as<std::string>();
        Component::Create(type, component_node, node);
    }

    for (const YAML::Node& child_node : p_data["children"]) {
        Ref<Node> new_node;

        if (child_node["scene"]) {
            const std::string subscene_path = child_node["scene"].as<std::string>();
            new_node = ResourceManager::Load<Scene>(subscene_path)->Instantiate();
        } else {
            new_node = FromData(child_node);
        }

        node->AddChild(new_node);
    }

    return node;
}

// --- Resource interface ---
Scene Scene::Load(StringID p_id) {
    return Scene(std::make_shared<YAML::Node>(YAML::LoadFile(p_id)));
}

void Unload() {
}