#include "billboard_shader.hpp"

#include <gauge/core/app.hpp>
#include <gauge/renderer/vulkan/graphics_pipeline_builder.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/renderer/vulkan/shader_module.hpp>

using namespace Gauge;

extern App* gApp;

void BillboardShader::Initialize(const RendererVulkan& renderer) {
    id = "Billboard"_id;

    auto shader_module_result = ShaderModule::FromFile(renderer.ctx, "shaders/billboard.spv");
    CHECK(shader_module_result);
    ShaderModule shader_module = shader_module_result.value();
    renderer.SetDebugName((uint64_t)shader_module.handle, VK_OBJECT_TYPE_SHADER_MODULE, std::format("{} shader module", id));

    builder =
        GraphicsPipelineBuilder(id)
            .SetVertexStage(shader_module.handle, "VertexMain")
            .SetFragmentStage(shader_module.handle, "FragmentMain")
            .AddDescriptorSetLayout(renderer.global_descriptor.layout)
            .AddDescriptorSetLayout(renderer.frames_in_flight[0].descriptor_set.GetLayout())
            .AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(BillboardShader::PushConstants))
            .SetImageFormat(renderer.offscreen ? VK_FORMAT_R8G8B8A8_SRGB : renderer.swapchain.image_format)
            .SetSampleCount(RendererVulkan::SampleCountFromMSAA(gApp->project_settings.msaa_level))
            .EnableDepthTest(true)
            .SetCullMode(VK_CULL_MODE_NONE)
            .SetTransparency(true);
    pipeline = builder.Build(renderer).value();
}

void BillboardShader::Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const {
    const VkDescriptorSet sets[] = {
        renderer.global_descriptor.set.handle,
        renderer.GetCurrentFrame().descriptor_set.handle,
    };

    vkCmdBindDescriptorSets(
        cmd.GetHandle(),
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.layout,
        0,
        2,
        sets,
        0,
        nullptr);

    PushConstants pcs;
    pcs.camera_index = 0;

    cmd.BindPipeline(pipeline);
    for (const DrawObject& object : objects) {
        pcs.world_position = object.world_position;
        pcs.size = object.size;
        pcs.material = *renderer.resources.materials.Get(object.material);
        pcs.node_handle = object.node_handle;
        vkCmdPushConstants(cmd.GetHandle(), pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BillboardShader::PushConstants), &pcs);
        vkCmdDraw(cmd.GetHandle(), 6, 1, 0, 0);
    }
}

void BillboardShader::Clear() {
    objects.clear();
}