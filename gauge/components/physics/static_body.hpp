#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/core/handle.hpp>
#include <gauge/physics/physics.hpp>

namespace Gauge {

struct StaticBody : public Component {
    Physics::ShapeHandle shape{};
    Physics::ShapeHandle body{};

   public:
    virtual void Initialize() final override;
    virtual void Update(float delta) final override;

    StaticBody(Physics::ShapeHandle p_shape = 0, Physics::ShapeHandle p_body = 0)
        : Component(false, false), shape(p_shape) {}

    static void StaticInitialize() {}
};

}  // namespace Gauge