#include "shader.hpp"

#include <gauge/renderer/vulkan/renderer_vulkan.hpp>

using namespace Gauge;

void Shader::Reload(const RendererVulkan& renderer) {
    vkDeviceWaitIdle(renderer.ctx.device);
    vkDestroyPipeline(renderer.ctx.device, pipeline.handle, nullptr);
    vkDestroyPipelineLayout(renderer.ctx.device, pipeline.layout, nullptr);
    Initialize(renderer);
}