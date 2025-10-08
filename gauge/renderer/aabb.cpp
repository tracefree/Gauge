#include "aabb.hpp"

#include <gauge/math/transform.hpp>

using namespace Gauge;

bool AABB::IsPointInside(Vec3 p_point) const {
    return (
        p_point.x <= (position.x + extent.x) && p_point.x >= (position.x - extent.x) &&
        p_point.y <= (position.y + extent.y) && p_point.y >= (position.y - extent.y) &&
        p_point.z <= (position.z + extent.z) && p_point.z >= (position.z - extent.z));
}

void AABB::Grow(Vec3 p_point) {
    if (!valid) {
        position = p_point;
        valid = true;
        return;
    }
    if (p_point.x > (position.x + extent.x)) {
        position.x = ((position.x - extent.x) + p_point.x) / 2.0;
        extent.x = p_point.x - position.x;
    } else if (p_point.x < (position.x - extent.x)) {
        position.x = ((position.x + extent.x) + p_point.x) / 2.0;
        extent.x = position.x - p_point.x;
    }
    if (p_point.y > (position.y + extent.y)) {
        position.y = ((position.y - extent.y) + p_point.y) / 2.0;
        extent.y = p_point.y - position.y;
    } else if (p_point.y < (position.y - extent.y)) {
        position.y = ((position.y + extent.y) + p_point.y) / 2.0;
        extent.y = position.y - p_point.y;
    }
    if (p_point.z > (position.z + extent.z)) {
        position.z = ((position.z - extent.z) + p_point.z) / 2.0;
        extent.z = p_point.z - position.z;
    } else if (p_point.z < (position.z - extent.z)) {
        position.z = ((position.z + extent.z) + p_point.z) / 2.0;
        extent.z = position.z - p_point.z;
    }
}

void AABB::Grow(AABB p_other) {
    Grow(p_other.position + Vec3(p_other.extent.x, p_other.extent.y, p_other.extent.z));
    Grow(p_other.position + Vec3(p_other.extent.x, p_other.extent.y, -p_other.extent.z));
    Grow(p_other.position + Vec3(p_other.extent.x, -p_other.extent.y, p_other.extent.z));
    Grow(p_other.position + Vec3(p_other.extent.x, -p_other.extent.y, -p_other.extent.z));
    Grow(p_other.position + Vec3(-p_other.extent.x, p_other.extent.y, p_other.extent.z));
    Grow(p_other.position + Vec3(-p_other.extent.x, p_other.extent.y, -p_other.extent.z));
    Grow(p_other.position + Vec3(-p_other.extent.x, -p_other.extent.y, p_other.extent.z));
    Grow(p_other.position + Vec3(-p_other.extent.x, -p_other.extent.y, -p_other.extent.z));
}