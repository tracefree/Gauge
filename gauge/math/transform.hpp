#pragma once

#include <gauge/math/common.hpp>
#include <gauge/renderer/aabb.hpp>

namespace Gauge {

struct Transform {
    static const Transform IDENTITY;

    Vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{glm::identity<Quaternion>()};
    float scale{1.0f};

    Mat4 GetMatrix() const;
    const Transform operator*(Transform const& rhs) const;
    const AABB operator*(AABB const& rhs) const;

    Transform() {}
    Transform(Transform const& p_transform);
    Transform(Vec3 p_position, glm::quat p_rotation, float p_scale);
    ~Transform() {}
};

}  // namespace Gauge