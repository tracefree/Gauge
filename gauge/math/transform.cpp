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
