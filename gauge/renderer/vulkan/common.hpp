#pragma once

#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>
#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

#include <vk_mem_alloc.h>

struct VulkanContext {
    vkb::Instance instance{};
    vkb::PhysicalDevice physical_device{};
    vkb::Device device{};
    VmaAllocator allocator{};
    VkQueue graphics_queue{};
};

struct Pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkPipelineBindPoint bind_point;
};

struct BufferAllocation {
    VkBuffer buffer{};
    VkDeviceAddress address{};
    VmaAllocation allocation{};
    VmaAllocationInfo info{};
};

struct GPUMesh {
    uint index_count;
    BufferAllocation index_buffer{};
    BufferAllocation vertex_buffer{};
};

struct ImageAllocation {
    VmaAllocation allocation{};
    VmaAllocationInfo info{};
};

struct GPUImage {
    VkImage image{};
    VkImageView view{};
    VkFormat format{};
    VkExtent3D extent{};
    VkDeviceMemory memory{};
    ImageAllocation allocation{};
};

#define VK_CHECK(result, message)                 \
    if (result != VK_SUCCESS) [[unlikely]] {      \
        std::println("Gauge Error: {}", message); \
    }

#define VK_CHECK_RET(result, return_value)                                                                    \
    if (result != VK_SUCCESS) [[unlikely]] {                                                                  \
        return std::unexpected(std::format("{}. Vulkan result: {}.", return_value, string_VkResult(result))); \
    }
