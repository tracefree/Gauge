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

    inline uint ToUint() {
        return index;  // TODO: take generation into account
    }

    static inline Handle<T> FromUint(uint p_handle) {
        return Handle<T>{
            .generation = 0,  // TODO
            .index = uint16_t(p_handle),
        };
    }
};

}  // namespace Gauge