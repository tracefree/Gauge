#pragma once

#include <gauge/renderer/shaders/shader.hpp>
#include <vector>

namespace Gauge {

class PBRShader : public Shader {
   public:
    struct PushConstants {
        Mat4 model_matrix;
        VkDeviceAddress vertex_buffer_address;
        GPUMaterial material;
        uint camera_id;
        uint node_handle;
    };

    struct DrawObject {
        Handle<GPUMesh> primitive;
        Handle<GPUMaterial> material;
        Transform transform;
        uint node_handle;
    };

    std::vector<DrawObject> objects;

   public:
    virtual void Initialize(const RendererVulkan& renderer) override;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const override;
    virtual void Clear() override;

    PBRShader() {}
    ~PBRShader() {}
};

}  // namespace Gauge