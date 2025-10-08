#pragma once

#include <gauge/renderer/aabb.hpp>
#include <gauge/renderer/shaders/shader.hpp>

#include <vector>

namespace Gauge {

class AABBShader : public Shader {
   public:
    struct PushConstants {
        Mat4 model_matrix;
        VkDeviceAddress vertex_buffer_address;
        uint camera_index;
        uint padding;
        Vec3 extent;
    };

    struct DrawObject {
        AABB aabb;
        Transform transform;
    };

    std::vector<DrawObject> objects;

   public:
    virtual void Initialize(const RendererVulkan& renderer) override;
    virtual void Draw(RendererVulkan& renderer, const CommandBufferVulkan& cmd) const override;
    virtual void Clear() override;

    AABBShader() {}
    ~AABBShader() {}
};

}  // namespace Gauge