#pragma once

#include <gauge/components/component.hpp>
#include <gauge/math/common.hpp>

namespace Gauge {

struct CharacterController : public Component {
    Vec3 velocity = Vec3::ZERO;

    virtual void Initialize() final override;
    virtual void Update(float delta) final override;

    static void StaticInitialize() {}
    COMPONENT_FACTORY_HEADER(CharacterController);
};

}  // namespace Gauge