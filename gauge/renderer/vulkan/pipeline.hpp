#pragma once

#include <volk.h>
#include <vulkan/vulkan_core.h>

struct Pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkPipelineBindPoint bind_point;
};