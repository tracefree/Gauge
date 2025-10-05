#pragma once

#include <gauge/renderer/shaders/shader.hpp>
#include <vector>

namespace Gauge {

class AABBShader : public Shader {
   public:
    struct PushConstants {
        Vec3 position;
        uint camera_index;
        Vec3 extent;
        float _padding1;
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