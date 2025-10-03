#pragma once

#include <gauge/math/common.hpp>

namespace Gauge {

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

}  // namespace Gauge