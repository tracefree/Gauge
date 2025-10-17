#include "billboard.hpp"

#include <gauge/core/app.hpp>
#include <gauge/math/transform.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/shaders/billboard/billboard_shader.hpp>
#include <gauge/renderer/shaders/gizmo/gizmo_shader.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/scene/node.hpp>

#include <yaml-cpp/yaml.h>
#include <print>

using namespace Gauge;

extern App* gApp;

void Billboard::Draw() {
    auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
    renderer->GetShader<BillboardShader>()->objects.emplace_back(
        BillboardShader::DrawObject{
            .material = material,
            .world_position = node ? node->GetGlobalTransform().position : Vec3::ZERO,
            .size = size,
            .node_handle = node ? node->handle.ToUint() : 0,
        });
}

COMPONENT_FACTORY_IMPL(Billboard, billboard) {
}