#include "gizmo_shader.hpp"
#include <sys/types.h>

#include <gauge/core/app.hpp>
#include <gauge/renderer/vulkan/graphics_pipeline_builder.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>
#include <gauge/renderer/vulkan/shader_module.hpp>

using namespace Gauge;

extern App* gApp;

void GizmoShader::Initialize(const RendererVulkan& renderer) {
    id = "Gizmo"_id;
    path = "shaders/gizmo.spv";

    auto shader_module_result = ShaderModule::FromFile(renderer.ctx, path);
    CHECK(shader_module_result);
    ShaderModule shader_module = shader_module_result.value();
    renderer.SetDebugName((uint64_t)shader_module.handle, VK_OBJECT_TYPE_SHADER_MODULE, std::format("{} shader module", id));

    builder =
        GraphicsPipelineBuilder(id)
            .SetVertexStage(shader_module.handle, "VertexMain")
            .SetFragmentStage(shader_module.handle, "FragmentMain")
            .AddDescriptorSetLayout(renderer.global_descriptor.layout)
            .AddDescriptorSetLayout(renderer.frames_in_flight[0].descriptor_set.GetLayout())
            .AddPushConstantRange((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), sizeof(PushConstants))
            .EnableDepthTest(false)
            .SetImageFormat(renderer.offscreen ? VK_FORMAT_R8G8B8A8_SRGB : renderer.swapchain.image_format)
            .SetSampleCount(RendererVulkan::SampleCountFromMSAA(gApp->project_settings.msaa_level));
    pipeline = builder.Build(renderer).value();
}

void GizmoShader::Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const {
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
    pcs.camera_id = 0;
    cmd.BindPipeline(pipeline);
    for (const DrawObject& object : objects) {
        pcs.model_matrix = object.transform.GetMatrix();
        pcs.material = *renderer.resources.materials.Get(object.material);
        GPUMesh& mesh = *renderer.resources.meshes.Get(object.primitive);
        pcs.vertex_buffer_address = mesh.vertex_buffer.address;
        pcs.node_handle = object.node_handle;
        vkCmdPushConstants(cmd.GetHandle(), pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcs);
        vkCmdBindIndexBuffer(cmd.GetHandle(), mesh.index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd.GetHandle(), mesh.index_count, 1, 0, 0, 0);
    }
}

void GizmoShader::Clear() {
    objects.clear();
}