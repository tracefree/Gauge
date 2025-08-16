#pragma once

#include <expected>
#include <memory>

namespace Gauge {

template <typename T = void>
using Result = std::expected<T, std::string>;

template <class E>
using Error = std::unexpected<E>;

template <typename T>
using Ref = std::shared_ptr<T>;

// Resource ID
typedef uint32_t RID;

#define CHECK(result)                              \
    if (!result) [[unlikely]] {                    \
        std::println("Error: {}", result.error()); \
    }

#define CHECK_RET(result)             \
    if (!result) [[unlikely]] {       \
        return Error(result.error()); \
    }
}  // namespace Gauge