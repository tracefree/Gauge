#pragma once

#include <cstddef>
#include <cstdint>
namespace Gauge {

template <typename T>
struct Handle {
    uint16_t index{};
    uint16_t generation{};

    inline operator uint() const {
        return index;
    }

    size_t hash() const {
        return std::hash<uint>()(index);
    }

    // size_t operator()
};

}  // namespace Gauge