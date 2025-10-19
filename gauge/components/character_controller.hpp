#pragma once

#include <gauge/components/component.hpp>
#include <gauge/math/common.hpp>
#include <gauge/physics/character.hpp>

#include <memory>

namespace Gauge {

namespace Physics {
class Character;
}

struct CharacterController final : public Component {
    std::unique_ptr<Physics::Character> character;
    Vec3 velocity = Vec3::ZERO;

    virtual void Initialize() final override;
    virtual void Update(float delta) final override;

    static void StaticInitialize() {}
    COMPONENT_FACTORY_HEADER(CharacterController);
};

}  // namespace Gauge