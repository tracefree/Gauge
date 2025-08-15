#pragma once

#include <gauge/common.hpp>

namespace Gauge {
struct CommandBuffer {
   public:
    virtual Result<> Begin() = 0;
    virtual Result<> End() = 0;
    // virtual void BeginRendering() = 0;
    // virtual void EndRendering() = 0;
};
}  // namespace Gauge