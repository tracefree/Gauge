#include "transform.hpp"
#include <cstdlib>

using namespace Gauge;

const Transform Transform::IDENTITY = Transform();

Transform::Transform(Vec3 p_position, glm::quat p_rotation, float p_scale) {
    position = p_position;
    rotation = p_rotation;
    scale = p_scale;
}

Transform::Transform(Transform const& p_transform) {
    position = p_transform.position;
    rotation = p_transform.rotation;
    scale = p_transform.scale;
}

Mat4 Transform::GetMatrix() const {
    return glm::translate(Mat4(1.0f), position) * glm::toMat4(rotation) * glm::scale(Mat4(1.0f), Vec3(scale));
}

const Transform Transform::operator*(Transform const& rhs) const {
    return Transform{
        position + rotation * (scale * rhs.position),
        rotation * rhs.rotation,
        scale * rhs.scale,
    };
}

// From "Real-Time Collision Detection" by Christer Ericson, 4.2.6
const AABB Transform::operator*(AABB const& rhs) const {
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