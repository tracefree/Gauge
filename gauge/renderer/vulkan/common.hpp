#pragma once

#include <gauge/math/transform.hpp>

#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>
#include "gauge/renderer/common.hpp"
#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

#include <vk_mem_alloc.h>

#include <string>
#include <vector>

namespace Gauge {

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

struct Allocation {
    VmaAllocation handle{};
    VmaAllocationInfo info{};
};

struct GPUBuffer {
    VkBuffer handle{};
    VkDeviceAddress address{};
    Allocation allocation{};
};

struct GPUMesh {
    uint index_count;
    GPUBuffer index_buffer{};
    GPUBuffer vertex_buffer{};
};

struct GPUImage {
    VkImage handle{};
    VkImageView view{};
    VkFormat format{};
    VkExtent3D extent{};
    // VkDeviceMemory memory{};
    Allocation allocation{};
};

struct Mesh {
    std::string name;
    GPUMesh data;
    GPUMaterial material;
};

struct Model {
    std::vector<Mesh> meshes;
    Transform transform{};
};

#define VK_CHECK(result, message)                 \
    if (result != VK_SUCCESS) [[unlikely]] {      \
        std::println("Gauge Error: {}", message); \
    }

#define VK_CHECK_RET(result, return_value)                                                          \
    if (result != VK_SUCCESS) [[unlikely]] {                                                        \
        return Error(std::format("{}. Vulkan result: {}.", return_value, string_VkResult(result))); \
    }

}  // namespace Gauge