#pragma once

#include <gauge/renderer/shaders/shader.hpp>
#include <vector>

namespace Gauge {

class PBRShader : public Shader {
   public:
    struct MousePosition {
        uint16_t x;
        uint16_t y;
    };

    struct PushConstants {
        Mat4 model_matrix;
        VkDeviceAddress vertex_buffer_address;
        uint sampler;
        uint material_index;
        uint camera_index;
        uint scene_index;
        MousePosition mouse_position;
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
    virtual void Initialize(RendererVulkan& renderer) override;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) override;
    virtual void Clear() override;

    PBRShader() {}
    ~PBRShader() {}
};

}  // namespace Gauge