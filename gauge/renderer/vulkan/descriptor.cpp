#include "descriptor.hpp"
#include <vulkan/vulkan_core.h>
#include "gauge/renderer/vulkan/common.hpp"

using namespace Gauge;

// --- DescriptorSet ---

VkDescriptorSet DescriptorSet::GetHandle() {
    return handle;
}

VkDescriptorSetLayout DescriptorSet::GetLayout() {
    return layout;
}

VkDescriptorPool DescriptorSet::GetPool() {
    return pool;
}

DescriptorSet::DescriptorSet(VkDescriptorSet p_set, VkDescriptorSetLayout p_layout, VkDescriptorPool p_pool) {
    handle = p_set,
    layout = p_layout,
    pool = p_pool;
}

Result<DescriptorSet> DescriptorSet::Create(const VulkanContext ctx, VkDescriptorSetLayout p_layout, VkDescriptorPool p_pool) {
    VkDescriptorSet set{};
    const VkDescriptorSetAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = p_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &p_layout};
    VK_CHECK_RET(vkAllocateDescriptorSets(
                     ctx.device,
                     &allocate_info,
                     &set),
                 "Could not create descriptor set");
    return DescriptorSet(set, p_layout, p_pool);
}

void DescriptorSet::WriteImage(
    const VulkanContext ctx,
    uint p_bind_point,
    uint p_element,
    VkImageView p_view,
    VkImageLayout p_layout) {
    VkDescriptorImageInfo image_info{
        .imageView = p_view,
        .imageLayout = p_layout,
    };
    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = handle,
        .dstBinding = p_bind_point,
        .dstArrayElement = p_element,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &image_info,
    };
    vkUpdateDescriptorSets(ctx.device, 1, &write, 0, nullptr);
}

void DescriptorSet::WriteStorageBuffer(
    const VulkanContext ctx,
    uint p_bind_point,
    uint p_element,
    VkBuffer p_buffer,
    VkDeviceSize p_range,
    VkDeviceSize p_offset) {
    VkDescriptorBufferInfo buffer_info{
        .buffer = p_buffer,
        .range = p_range,
    };
    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = handle,
        .dstBinding = p_bind_point,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &buffer_info};
    vkUpdateDescriptorSets(ctx.device, 1, &write, 0, nullptr);
}

// --- DescriptorSetLayout Builder ---

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::AddBinding(
    VkDescriptorType p_type,
    uint p_count,
    VkShaderStageFlagBits p_shader_stages,
    VkSampler* p_immutable_samplers) {
    bindings.emplace_back(VkDescriptorSetLayoutBinding{
        .binding = (uint)bindings.size(),
        .descriptorType = p_type,
        .descriptorCount = p_count,
        .stageFlags = p_shader_stages,
        .pImmutableSamplers = p_immutable_samplers,
    });
    return *this;
}

DescriptorSetLayoutBuilder&
DescriptorSetLayoutBuilder::SetFlags(VkDescriptorSetLayoutCreateFlagBits p_flags) {
    flags = p_flags;
    return *this;
}

Result<VkDescriptorSetLayout> DescriptorSetLayoutBuilder::Build(const VulkanContext& ctx) {
    const VkDescriptorSetLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = flags,
        .bindingCount = (uint)bindings.size(),
        .pBindings = bindings.data()};
    VK_CHECK_RET(vkCreateDescriptorSetLayout(
                     ctx.device,
                     &layout_info,
                     nullptr,
                     &layout),
                 "Could not create descriptor set layout");
    return layout;
}