#pragma once

#include <gauge/common.hpp>

namespace JPH {
class Shape;
}

namespace Gauge {
namespace Physics {

class Shape {
   public:
    const Ref<JPH::Shape> jolt_shape;
};

}  // namespace Physics
}  // namespace Gauge