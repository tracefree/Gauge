#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/core/handle.hpp>

#include <vector>
#include "gauge/renderer/common.hpp"

namespace Gauge {

struct GPUMesh;

struct MeshInstance : public Component {
    struct Surface {
        Handle<GPUMesh> primitive;
        Handle<GPUMaterial> material;
    };
    std::vector<Surface> surfaces;

   public:
    virtual void Draw() override;

    COMPONENT_FACTORY_HEADER(MeshInstance)
};

}  // namespace Gauge