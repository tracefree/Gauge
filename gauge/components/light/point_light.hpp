#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include "gauge/math/common.hpp"

namespace Gauge {

struct PointLight : public Component {
    Vec3 color = Vec3::ONE;
    float intensity = 1.0f;
    float range = 5.0f;

   public:
    virtual void Draw() override;
    virtual void Update(float delta) override;

    static void StaticInitialize() {}

    COMPONENT_FACTORY_HEADER(PointLight)
};

}  // namespace Gauge