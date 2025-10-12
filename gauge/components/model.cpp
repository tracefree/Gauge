#include "model.hpp"

#include <gauge/core/resource_manager.hpp>
#include <gauge/renderer/gltf.hpp>
#include <gauge/scene/node.hpp>
#include <gauge/scene/yaml.hpp>

#include <print>
#include <string>

using namespace Gauge;

void ModelComponent::Initialize() {
    const auto model_node = ResourceManager::Load<glTF>(path)->CreateNode().value();
    node->AddChild(model_node);
}

COMPONENT_FACTORY_IMPL(ModelComponent, model) {
    path = p_data["path"].as<std::string>();
    generate_collisions = p_data["collisions"].IsDefined() && p_data["collisions"].as<bool>();
}