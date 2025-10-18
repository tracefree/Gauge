#pragma once

#include <gauge/renderer/shaders/shader.hpp>

#include <vector>

namespace Gauge {

class BillboardShader : public Shader {
   public:
    struct PushConstants {
        Vec3 world_position;
        uint camera_index;
        Vec2 size;
        GPUMaterial material;
        uint node_handle;
    };

    struct DrawObject {
        Handle<GPUMaterial> material;
        Vec3 world_position;
        Vec2 size;
        uint node_handle;
    };

    std::vector<DrawObject> objects;

   public:
    virtual void Initialize(const RendererVulkan& renderer) override;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const override;
    virtual void Clear() override;

    BillboardShader() {}
    ~BillboardShader() {}
};

}  // namespace Gauge