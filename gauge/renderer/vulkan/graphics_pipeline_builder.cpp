#include "graphics_pipeline_builder.hpp"
#include <vulkan/vulkan_core.h>
#include "gauge/renderer/vulkan/common.hpp"

using namespace Gauge;

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddPushConstantRange(VkShaderStageFlags p_shader_stage_flags, uint p_size) {
    push_constant_ranges.emplace_back(VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = p_size,
    });
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::AddDescriptorSetLayout(VkDescriptorSetLayout p_descriptor_set_layout) {
    descriptor_set_layouts.push_back(p_descriptor_set_layout);
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetVertexStage(VkShaderModule p_shader_module, const char* p_entry_point) {
    vertex_stage = ShaderStage{
        .shader_module = p_shader_module,
        .entry_point = p_entry_point,
    };
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetFragmentStage(VkShaderModule p_shader_module, const char* p_entry_point) {
    fragment_stage = ShaderStage{
        .shader_module = p_shader_module,
        .entry_point = p_entry_point,
    };
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::SetImageFormat(VkFormat p_format) {
    image_format = p_format;
    return *this;
}

Result<Pipeline>
GraphicsPipelineBuilder::build(const VulkanContext& ctx) const {
    Pipeline pipeline{};

    const VkPipelineShaderStageCreateInfo vertex_shader_stage_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertex_stage.shader_module,
        .pName = vertex_stage.entry_point,
    };

    const VkPipelineShaderStageCreateInfo fragment_shader_stage_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragment_stage.shader_module,
        .pName = fragment_stage.entry_point,
    };

    const VkPipelineShaderStageCreateInfo shader_stage_infos[] = {vertex_shader_stage_info, fragment_shader_stage_info};

    const VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    const VkPipelineDynamicStateCreateInfo dynamic_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states,
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    const VkPipelineViewportStateCreateInfo viewport_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo msaa_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment_state{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint)descriptor_set_layouts.size(),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = (uint)push_constant_ranges.size(),
        .pPushConstantRanges = push_constant_ranges.data(),
    };

    VK_CHECK_RET(vkCreatePipelineLayout(ctx.device, &pipeline_layout_info, nullptr, &pipeline.layout),
                 "Could not create pipeline layout");
    // SetDebugName((uint64_t)pipeline.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, std::format("{} layout", p_name));

    const VkPipelineRenderingCreateInfo pipeline_rendering_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &image_format,
        .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
    };

    const VkGraphicsPipelineCreateInfo pipeline_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_info,
        .stageCount = 2,
        .pStages = shader_stage_infos,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &msaa_info,
        .pDepthStencilState = &depth_state_info,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = pipeline.layout,
    };

    VK_CHECK_RET(vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline.handle),
                 "Could not create graphics pipeline");
    //  SetDebugName((uint64_t)pipeline.handle, VK_OBJECT_TYPE_PIPELINE, p_name);

    return pipeline;
}

GraphicsPipelineBuilder::GraphicsPipelineBuilder(std::string p_name) {
    name = p_name;
}