#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/core/handle.hpp>

#include <vector>
#include "gauge/core/string_id.hpp"
#include "gauge/renderer/common.hpp"

namespace Gauge {

struct GPUMesh;

struct MeshInstance final : public Component {
    struct Surface {
        Handle<GPUMesh> primitive;
        Handle<GPUMaterial> material;
        StringID shader_id;
    };
    std::vector<Surface> surfaces;

   public:
    static void StaticInitialize() {}
    virtual void Draw() override;

    COMPONENT_FACTORY_HEADER(MeshInstance)
};

}  // namespace Gauge