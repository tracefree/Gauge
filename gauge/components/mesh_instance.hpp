#pragma once

#include "gauge/components/component.hpp"
#include "gauge/components/drawable.hpp"

namespace Gauge {

struct MeshInstance : public Component, public IDrawable {
    // GPUMesh mesh{};

   public:
    virtual void Draw() const override;

    MeshInstance() {}
    ~MeshInstance() {}
};

}  // namespace Gauge