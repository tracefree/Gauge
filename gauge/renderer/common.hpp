#pragma once

#include <vulkan/vulkan_core.h>
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "vk_mem_alloc.h"

struct Vertex {
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 tangent;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint> indices;

    VkDeviceAddress vertex_buffer;
    VkDeviceAddress index_buffer;
};