#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/core/handle.hpp>
#include <gauge/renderer/aabb.hpp>

namespace Gauge {

struct AABBGizmo : public Component {
    AABB aabb{};

   public:
    virtual void Draw() override;
    AABBGizmo() {}
    AABBGizmo(AABB p_aabb) : Component(false, false), aabb(p_aabb) {}
};

}  // namespace Gauge