#include "mesh_instance.hpp"

#include <gauge/scene/node.hpp>
#include "gauge/core/app.hpp"
#include "gauge/math/transform.hpp"
#include "gauge/renderer/renderer.hpp"

#include <yaml-cpp/yaml.h>

using namespace Gauge;

extern App* gApp;

void MeshInstance::Draw() {
    for (const auto& surface : surfaces) {
        gApp->renderer->draw_objects.emplace_back(
            Renderer::DrawObject{
                .primitive = surface.primitive,
                .material = surface.material,
                .transform = node ? node->global_transform : Transform(),
                .node_handle = node ? node->handle.ToUint() : 0,
            });
    }
}

COMPONENT_FACTORY_IMPL(MeshInstance, mesh_instance) {
}