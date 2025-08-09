#pragma once

#include <vulkan/vk_enum_string_helper.h>
#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

struct VulkanContext {
    vkb::Instance instance{};
    vkb::PhysicalDevice physical_device{};
    vkb::Device device{};
    VkQueue graphics_queue{};
};

#define VK_CHECK(result, message)                 \
    if (result != VK_SUCCESS) [[unlikely]] {      \
        std::println("Gauge Error: {}", message); \
    }

#define VK_CHECK_RET(result, return_value)                                                                    \
    if (result != VK_SUCCESS) [[unlikely]] {                                                                  \
        return std::unexpected(std::format("{}. Vulkan result: {}.", return_value, string_VkResult(result))); \
    }
