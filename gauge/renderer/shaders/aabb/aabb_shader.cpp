#include "aabb_shader.hpp"

#include <gauge/core/app.hpp>
#include <gauge/renderer/vulkan/graphics_pipeline_builder.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/renderer/vulkan/shader_module.hpp>

using namespace Gauge;

extern App* gApp;

void AABBShader::Initialize(RendererVulkan& renderer) {
    id = "AABB"_id;

    auto shader_module_result = ShaderModule::FromFile(renderer.ctx, "shaders/aabb.spv");
    CHECK(shader_module_result);
    ShaderModule shader_module = shader_module_result.value();

    pipeline =
        GraphicsPipelineBuilder(id)
            .SetVertexStage(shader_module.handle, "VertexMain")
            .SetFragmentStage(shader_module.handle, "FragmentMain")
            .AddDescriptorSetLayout(renderer.global_descriptor.layout)
            .AddDescriptorSetLayout(renderer.frames_in_flight[0].descriptor_set.GetLayout())
            .AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstants))
            .SetImageFormat(renderer.offscreen ? VK_FORMAT_R8G8B8A8_SRGB : renderer.swapchain.image_format)
            .SetSampleCount(RendererVulkan::SampleCountFromMSAA(gApp->project_settings.msaa_level))
            .SetLineTopology(true)
            .EnableDepthTest(false)
            .Build(renderer.ctx)
            .value();
}

void AABBShader::Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) {
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
        const AABB transformed_aabb = object.transform * object.aabb;
        pcs.position = transformed_aabb.position;
        pcs.extent = transformed_aabb.extent;
        vkCmdPushConstants(cmd.GetHandle(), pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pcs);
        vkCmdDraw(cmd.GetHandle(), 24, 1, 0, 0);
    }
}

void AABBShader::Clear() {
    objects.clear();
}