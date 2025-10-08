#pragma once

#include <gauge/math/common.hpp>
#include <gauge/math/transform.hpp>

namespace Gauge {

struct Transform;

class AABB {
   public:
    Vec3 position{};
    Vec3 extent{};

   protected:
    bool valid = false;

   public:
    bool IsPointInside(Vec3 p_point) const;
    void Grow(Vec3 p_point);
    void Grow(AABB p_other);
    inline bool IsValid() { return valid; }

    AABB() {}
    AABB(Vec3 position, Vec3 extent) : position(position), extent(extent), valid(true) {}
};

// From "Real-Time Collision Detection" by Christer Ericson, 4.2.6
template <>
const inline AABB Transform::operator*(AABB const& rhs) const {
    AABB aabb(position, Vec3());
    Mat3 rotation_matrix = glm::toMat3(rotation);
    for (uint i = 0; i < 3; ++i) {
        for (uint j = 0; j < 3; ++j) {
            aabb.position[i] += rotation_matrix[j][i] * rhs.position[j];
            aabb.extent[i] += std::abs(rotation_matrix[j][i]) * rhs.extent[j];
        }
    }
    aabb.position *= scale;
    aabb.extent *= scale;
    return aabb;
}

}  // namespace Gauge