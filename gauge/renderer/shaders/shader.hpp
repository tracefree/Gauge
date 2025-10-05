#pragma once

#include <gauge/core/string_id.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/graphics_pipeline_builder.hpp>

namespace Gauge {

struct RendererVulkan;
struct CommandBufferVulkan;

class Shader {
   public:
    StringID id;
    StringID path;
    Pipeline pipeline;
    GraphicsPipelineBuilder builder;

   public:
    virtual void Initialize(const RendererVulkan& renderer) = 0;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const = 0;
    virtual void Clear() = 0;

    void Reload(const RendererVulkan& renderer);

    Shader() {}
    ~Shader() {}
};

template <typename S>
concept IsShader = requires(S shader) {
    std::is_base_of_v<Shader, S>;
};

}  // namespace Gauge