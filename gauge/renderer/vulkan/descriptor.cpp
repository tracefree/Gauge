#include "descriptor.hpp"
#include "gauge/renderer/vulkan/common.hpp"

using namespace Gauge;

DescriptorSetLayoutBuilder&
DescriptorSetLayoutBuilder::AddBinding(
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