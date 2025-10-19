#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/core/handle.hpp>
#include <gauge/core/string_id.hpp>
#include <gauge/renderer/common.hpp>

namespace Gauge {

struct Billboard final : public Component {
   public:
    Handle<GPUMaterial> material;
    Vec2 size = Vec2(50.0f);

   public:
    virtual void Draw() override;

    Billboard(Vec2 p_size) : size(p_size) {}

    static void StaticInitialize() {}
    COMPONENT_FACTORY_HEADER(Billboard)
};

}  // namespace Gauge