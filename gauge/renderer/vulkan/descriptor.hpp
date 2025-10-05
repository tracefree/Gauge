#pragma once

#include <gauge/common.hpp>

#include <volk.h>
#include <vulkan/vulkan_core.h>

namespace Gauge {

struct VulkanContext;

namespace DescriptorPool {

Result<VkDescriptorPool> Create(const VulkanContext& ctx, const std::vector<VkDescriptorPoolSize>& p_pool_sizes, VkDescriptorPoolCreateFlagBits p_flags, uint p_max_sets);
}

struct DescriptorSet {
   public:
    VkDescriptorSet handle{};

   private:
    VkDescriptorPool pool{};
    VkDescriptorSetLayout layout{};

   public:
    VkDescriptorSet GetHandle() const;
    VkDescriptorSetLayout GetLayout() const;
    VkDescriptorPool GetPool() const;

    void WriteImage(const VulkanContext& ctx, uint p_bind_point, uint p_element, VkImageView p_view, VkImageLayout p_layout);
    void WriteUniformBuffer(const VulkanContext& ctx, uint p_bind_point, uint p_element, VkBuffer p_buffer, VkDeviceSize p_range, VkDeviceSize p_offset = 0);
    void WriteStorageBuffer(const VulkanContext& ctx, uint p_bind_point, uint p_element, VkBuffer p_buffer, VkDeviceSize p_range, VkDeviceSize p_offset = 0);

    static Result<DescriptorSet> Create(const VulkanContext& ctx, VkDescriptorSetLayout p_layout, VkDescriptorPool p_pool);

    DescriptorSet(VkDescriptorSet p_set = VK_NULL_HANDLE, VkDescriptorSetLayout p_layout = VK_NULL_HANDLE, VkDescriptorPool p_pool = VK_NULL_HANDLE);
};

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