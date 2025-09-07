#pragma once

#include <gauge/common.hpp>
#include <gauge/components/component.hpp>
#include <gauge/core/handle.hpp>

#include <vector>
#include "gauge/renderer/common.hpp"

namespace Gauge {

struct MeshInstance : public Component {
    struct Surface {
        RID primitive;
        Handle<GPUMaterial> material;
    };
    std::vector<Surface> surfaces;

   public:
    virtual void Draw() override;

    MeshInstance() {}
    ~MeshInstance() {}
};

}  // namespace Gauge