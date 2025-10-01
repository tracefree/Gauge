#include "aabb_gizmo.hpp"

#include <gauge/core/app.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/scene/node.hpp>

using namespace Gauge;

extern App* gApp;

void Gauge::AABBGizmo::Draw() {
    gApp->renderer->draw_aabbs.emplace_back(Renderer::DrawAABB{
        .aabb = aabb,
        .transform = node ? node->GetGlobalTransform() : Transform()});
}