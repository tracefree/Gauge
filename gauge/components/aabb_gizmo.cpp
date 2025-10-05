#include "aabb_gizmo.hpp"

#include <gauge/core/app.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/shaders/aabb/aabb_shader.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/scene/node.hpp>

using namespace Gauge;

extern App* gApp;

void Gauge::AABBGizmo::Draw() {
    auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
    renderer->GetShader<AABBShader>()->objects.emplace_back(
        AABBShader::DrawObject{
            .aabb = aabb,
            .transform = node ? node->GetGlobalTransform() : Transform()});
}