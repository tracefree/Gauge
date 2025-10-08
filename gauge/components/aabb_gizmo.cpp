#include "aabb_gizmo.hpp"

#include <gauge/core/app.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/shaders/debug_line/debug_line_shader.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/scene/node.hpp>

using namespace Gauge;

extern App* gApp;

void Gauge::AABBGizmo::Draw() {
    const AABB transformed_aabb = node->GetGlobalTransform() * aabb;
    Mat4 transform{};
    transform[0][0] = transformed_aabb.extent.x;
    transform[1][1] = transformed_aabb.extent.y;
    transform[2][2] = transformed_aabb.extent.z;
    transform[3] = Vec4(transformed_aabb.position, 1.0);
    auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
    renderer->GetShader<DebugLineShader>()->objects.emplace_back(
        DebugLineShader::DrawObject{
            .mesh = renderer->resources.debug_mesh_box,
            .transform = transform,
            .color = Vec4(1.0, 0.5, 0.0, 1.0),
        });
}