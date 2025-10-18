#pragma once

#include <gauge/renderer/aabb.hpp>
#include <gauge/renderer/shaders/shader.hpp>

#include <vector>

namespace Gauge {

class DebugLineShader : public Shader {
   public:
    struct PushConstants {
        Mat4 model_matrix;
        VkDeviceAddress vertex_buffer_address;
        uint camera_index;
        Vec4 color;
    };

    struct DrawObject {
        Handle<GPUMesh> mesh;
        Mat4 transform;
        Vec4 color;
    };

    std::vector<DrawObject> objects;

   public:
    virtual void Initialize(const RendererVulkan& renderer) override;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const override;
    virtual void Clear() override;

    DebugLineShader() {}
    ~DebugLineShader() {}
};

}  // namespace Gauge