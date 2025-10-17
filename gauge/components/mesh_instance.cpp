#include "mesh_instance.hpp"

#include <gauge/core/app.hpp>
#include <gauge/math/transform.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/shaders/gizmo/gizmo_shader.hpp>
#include <gauge/renderer/shaders/pbr/pbr_shader.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/scene/node.hpp>

#include <yaml-cpp/yaml.h>

using namespace Gauge;

extern App* gApp;

void MeshInstance::Draw() {
    for (const auto& surface : surfaces) {
        auto renderer = static_cast<RendererVulkan*>(&(*gApp->renderer));
        auto shader_name = std::string(surface.shader_id);
        if (shader_name == "PBR") {
            renderer->GetShader<PBRShader>()->objects.emplace_back(
                PBRShader::DrawObject{
                    .primitive = surface.primitive,
                    .material = surface.material,
                    .transform = node ? node->global_transform : Transform(),
                    .node_handle = node ? node->handle.ToUint() : 0,
                });
        } else if (shader_name == "Gizmo") {
            renderer->GetShader<GizmoShader>()->objects.emplace_back(
                GizmoShader::DrawObject{
                    .primitive = surface.primitive,
                    .material = surface.material,
                    .transform = node ? node->global_transform : Transform(),
                    .node_handle = node ? node->handle.ToUint() : 0,
                });
        }
    }
}

COMPONENT_FACTORY_IMPL(MeshInstance, mesh_instance) {
}