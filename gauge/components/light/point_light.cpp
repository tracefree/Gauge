#include "point_light.hpp"

#include <gauge/core/app.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/scene/node.hpp>
#include <gauge/scene/yaml.hpp>

using namespace Gauge;

extern App* gApp;

void PointLight::Update(float delta) {
    auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
    auto& scene = renderer->render_state.scenes[0];
    scene.point_lights[scene.active_point_lights] = GPUPointLight{
        .position = node->GetGlobalTransform().position,
        .range = range,
        .color = color,
        .intensity = intensity,
    };
    scene.active_point_lights += 1;
}

void PointLight::Draw() {
}

COMPONENT_FACTORY_IMPL(PointLight, point_light) {
    if (p_data["range"]) {
        range = p_data["range"].as<float>();
    }
    if (p_data["color"]) {
        color = p_data["color"].as<Vec3>();
    }
    if (p_data["intensity"]) {
        intensity = p_data["intensity"].as<float>();
    }
}