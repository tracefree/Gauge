#include "mesh_instance.hpp"
#include "gauge/core/app.hpp"
#include "gauge/math/transform.hpp"
#include "gauge/renderer/renderer.hpp"

using namespace Gauge;

extern App* gApp;

void MeshInstance::Draw() {
    for (const auto& surface : surfaces) {
        gApp->renderer->draw_objects.emplace_back(
            Renderer::DrawObject{
                .primitive = surface.primitive,
                .material = surface.material,
                .transform = Transform{},
            });
    }
}