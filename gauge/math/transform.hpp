#pragma once

#include <gauge/math/common.hpp>

namespace Gauge {

struct Transform {
    static const Transform IDENTITY;

    Vec3 position{0.0f, 0.0f, 0.0f};
    Quaternion rotation{glm::identity<Quaternion>()};
    float scale{1.0f};

    Mat4 GetMatrix() const;

    template <typename T>
    const T operator*(T const& rhs) const;

    Transform() {}
    Transform(Transform const& p_transform);
    Transform(Vec3 p_position, glm::quat p_rotation, float p_scale);
    ~Transform() {}
};

template <>
const inline Transform Transform::operator*(Transform const& rhs) const {
    return Transform{
        position + rotation * (scale * rhs.position),
        rotation * rhs.rotation,
        scale * rhs.scale,
    };
}

template <>
const inline Vec3 Transform::operator*(Vec3 const& rhs) const {
    return (scale * (rotation * rhs)) + position;
}

template <>
const inline Vec4 Transform::operator*(Vec4 const& rhs) const {
    return GetMatrix() * rhs;
}

}  // namespace Gauge