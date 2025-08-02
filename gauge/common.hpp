#pragma once

#include <print>

#define CHECK(result)                              \
    if (!result) [[unlikely]] {                    \
        std::println("Error: {}", result.error()); \
    }

#define CHECK_RET(result)                       \
    if (!result) [[unlikely]] {                 \
        return std::unexpected(result.error()); \
    }
