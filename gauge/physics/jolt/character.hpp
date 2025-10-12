#pragma once

#include <gauge/physics/character.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <memory>

namespace JPH {
class Shape;
class CharacterVirtual;
}  // namespace JPH

namespace Gauge {
namespace Physics {

class JoltCharacter : public Physics::Character {
    std::unique_ptr<JPH::CharacterVirtual> jolt_character;

   public:
    virtual void Initialize() final override;
    virtual void Finalize() final override;
    virtual void SetVelocity(Vec3 p_velocity) final override;
    virtual Vec3 GetPosition() const final override;
    virtual void Update(float delta) final override;
    virtual bool IsOnGround() const final override;
};

}  // namespace Physics
}  // namespace Gauge