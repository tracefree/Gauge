#include "pbr_shader.hpp"

#include <format>
#include <gauge/core/app.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/renderer/vulkan/shader_module.hpp>

using namespace Gauge;

extern App* gApp;

void PBRShader::Initialize(const RendererVulkan& renderer) {
    id = "PBR"_id;

    auto shader_module_result = ShaderModule::FromFile(renderer.ctx, "shaders/pbr.spv");

    CHECK(shader_module_result);
    ShaderModule shader_module = shader_module_result.value();
    renderer.SetDebugName((uint64_t)shader_module.handle, VK_OBJECT_TYPE_SHADER_MODULE, std::format("{} shader module", id));

    builder =
        GraphicsPipelineBuilder(id)
            .SetVertexStage(shader_module.handle, "VertexMain")
            .SetFragmentStage(shader_module.handle, "FragmentMain")
            .AddDescriptorSetLayout(renderer.global_descriptor.layout)
            .AddDescriptorSetLayout(renderer.frames_in_flight[0].descriptor_set.GetLayout())
            .AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushConstants))
            .SetTransparency(true)
            .SetImageFormat(renderer.offscreen ? VK_FORMAT_R8G8B8A8_SRGB : renderer.swapchain.image_format)
            .SetSampleCount(RendererVulkan::SampleCountFromMSAA(gApp->project_settings.msaa_level));
    pipeline = builder.Build(renderer).value();
}

void PBRShader::Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const {
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
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    pcs.mouse_position = {.x = uint16_t(mx), .y = uint16_t(my)};
    pcs.sampler = 0;
    cmd.BindPipeline(pipeline);
    for (const DrawObject& draw_object : objects) {
        pcs.model_matrix = draw_object.transform.GetMatrix();
        const GPUMesh& mesh = *renderer.resources.meshes.Get(draw_object.primitive);
        pcs.vertex_buffer_address = mesh.vertex_buffer.address;
        pcs.material_index = draw_object.material.index;
        pcs.node_handle = draw_object.node_handle;
        vkCmdPushConstants(cmd.GetHandle(), pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);
        vkCmdBindIndexBuffer(cmd.GetHandle(), mesh.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd.GetHandle(), mesh.index_count, 1, 0, 0, 0);
    }
}

void PBRShader::Clear() {
    objects.clear();
}