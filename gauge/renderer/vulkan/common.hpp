#pragma once

#include <gauge/renderer/shaders/limits.h>
#include <gauge/math/common.hpp>
#include <gauge/math/transform.hpp>
#include <gauge/renderer/common.hpp>

#define VK_NO_PROTOTYPES 1
#include <volk.h>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#include <vk_mem_alloc.h>

#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Gauge {

struct VulkanContext {
    vkb::Instance instance{};
    vkb::PhysicalDevice physical_device{};
    vkb::Device device{};
    VmaAllocator allocator{};
    VkQueue graphics_queue{};
    int graphics_queue_family_index{};
    int graphics_queue_index{};
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
    void* mapped{};
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
    Allocation allocation{};
    int file_descriptor{};
};

struct GPUCamera {
    Mat4 view;
    Mat4 view_projection;
    Mat4 inverse_projection;
    Vec2 pixel_size;
    Vec2 _padding0;
};

struct GPUPointLight {
    Vec3 position;
    float range;
    Vec3 color;
    float intensity;
};

struct GPUScene {
    Vec3 ambient_light_color;
    float ambient_light_intensity;
    uint active_point_lights;
    float _padding1;
    float _padding2;
    float _padding3;
    GPUPointLight point_lights[MAX_POINT_LIGHTS];
};

struct GPUGlobals {
    float time;
    struct MousePosition {
        uint16_t x;
        uint16_t y;
    } mouse_position;
    float _padding2;
    float _padding3;
    GPUCamera cameras[MAX_CAMERAS];
    GPUScene scenes[MAX_SCENES];
};

struct Mesh {
    std::string name;
    GPUMesh data;
    GPU_PBRMaterial material;
};

struct Model {
    std::vector<Mesh> meshes;
    Transform transform{};
};

#define VK_CHECK(result, message)                 \
    if (result != VK_SUCCESS) [[unlikely]] {      \
        std::println("Gauge Error: {}", message); \
        assert(false);                            \
    }

#define VK_CHECK_RET(result, return_value)                                                          \
    if (result != VK_SUCCESS) [[unlikely]] {                                                        \
        return Error(std::format("{}. Vulkan result: {}.", return_value, string_VkResult(result))); \
    }

}  // namespace Gauge