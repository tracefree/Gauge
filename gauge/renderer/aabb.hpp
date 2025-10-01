#pragma once

#include <gauge/math/common.hpp>

namespace Gauge {

class AABB {
   public:
    Vec3 position{};
    Vec3 extent{};

   private:
    bool valid = false;

   public:
    bool IsPointInside(Vec3 p_point) const;
    void Grow(Vec3 p_point);
    void Grow(AABB p_other);
    inline bool IsValid() { return valid; }

    AABB() {}
    ~AABB() {}
};

}  // namespace Gauge