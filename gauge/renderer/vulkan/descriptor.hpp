#pragma once

#include <gauge/common.hpp>

#include <volk.h>
#include <vulkan/vulkan_core.h>

namespace Gauge {

struct VulkanContext;

struct DescriptorSetLayoutBuilder {
   private:
    VkDescriptorSetLayout layout{};
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    VkDescriptorSetLayoutCreateFlags flags{};

   public:
    DescriptorSetLayoutBuilder& AddBinding(VkDescriptorType p_type, uint p_count, VkShaderStageFlagBits p_shader_stage = VK_SHADER_STAGE_ALL, VkSampler* p_immutable_samplers = nullptr);
    DescriptorSetLayoutBuilder& SetFlags(VkDescriptorSetLayoutCreateFlagBits p_flags);
    Result<VkDescriptorSetLayout> Build(const VulkanContext& ctx);
};

}  // namespace Gauge