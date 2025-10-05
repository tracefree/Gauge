#pragma once

#include <gauge/renderer/vulkan/common.hpp>
#include "gauge/core/string_id.hpp"

namespace Gauge {

struct RendererVulkan;
struct CommandBufferVulkan;

class Shader {
   public:
    StringID id;
    Pipeline pipeline;

   public:
    virtual void Initialize(RendererVulkan& renderer) = 0;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) = 0;
    virtual void Clear() = 0;

    Shader() {}
    ~Shader() {}
};

template <typename S>
concept IsShader = requires(S shader) {
    std::is_base_of_v<Shader, S>;
};

}  // namespace Gauge