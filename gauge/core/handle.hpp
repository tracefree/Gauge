#pragma once

namespace Gauge {

template <typename T>
struct Handle {
    uint index{};
    uint generation{};
};

}  // namespace Gauge