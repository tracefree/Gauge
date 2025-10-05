#pragma once

#include <vulkan/vulkan_core.h>
#include <gauge/common.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <string>
#include <vector>

namespace Gauge {

struct RendererVulkan;

struct GraphicsPipelineBuilder {
   private:
    std::string name;

    std::vector<VkPushConstantRange> push_constant_ranges;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

    struct ShaderStage {
        VkShaderModule shader_module{};
        const char* entry_point{};
    };

    ShaderStage vertex_stage{};
    ShaderStage fragment_stage{};
    VkFormat image_format{};
    VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    VkCullModeFlagBits cull_mode = VK_CULL_MODE_BACK_BIT;
    bool transparency_enabled = false;
    bool line_topology_enabled = false;
    bool depth_test_enabled = true;

   public:
    GraphicsPipelineBuilder& AddPushConstantRange(VkShaderStageFlags p_shader_stage_flags, uint p_size);
    GraphicsPipelineBuilder& AddDescriptorSetLayout(VkDescriptorSetLayout p_descriptor_set_layout);
    GraphicsPipelineBuilder& SetVertexStage(VkShaderModule p_shader_module, const char* p_entry_point);
    GraphicsPipelineBuilder& SetFragmentStage(VkShaderModule p_shader_module, const char* p_entry_point);
    GraphicsPipelineBuilder& SetImageFormat(VkFormat p_format);
    GraphicsPipelineBuilder& SetSampleCount(VkSampleCountFlagBits p_sample_count);
    GraphicsPipelineBuilder& SetCullMode(VkCullModeFlagBits p_cull_mode);
    GraphicsPipelineBuilder& SetTransparency(bool p_enabled);
    GraphicsPipelineBuilder& SetLineTopology(bool p_enabled);
    GraphicsPipelineBuilder& EnableDepthTest(bool p_enabled = true);

    Result<Pipeline> Build(const RendererVulkan& renderer) const;

    GraphicsPipelineBuilder(std::string p_name = "");
    ~GraphicsPipelineBuilder() {}
};

}  // namespace Gauge