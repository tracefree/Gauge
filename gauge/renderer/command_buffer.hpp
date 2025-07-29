#pragma once

#include <expected>
#include <string>

namespace Gauge {
struct CommandBuffer {
   public:
    virtual std::expected<void, std::string> Begin() = 0;
    virtual std::expected<void, std::string> End() = 0;
};
}  // namespace Gauge