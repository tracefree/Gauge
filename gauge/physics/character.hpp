#pragma once

#include <gauge/math/common.hpp>

namespace Gauge {
namespace Physics {

class Character {
   public:
    virtual void Initialize() {}
    virtual void Finalize() {}
    virtual void SetVelocity(Vec3 p_velocity) {}
    virtual Vec3 GetPosition() const { return Vec3::ZERO; }
    virtual void Update(float delta) {}

    virtual bool IsOnGround() const { return false; }

    Character() {}
    virtual ~Character() {}
};

}  // namespace Physics
}  // namespace Gauge