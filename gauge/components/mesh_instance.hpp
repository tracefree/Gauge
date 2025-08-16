#pragma once

#include <gauge/common.hpp>
#include <vector>
#include "gauge/components/component.hpp"

namespace Gauge {

struct MeshInstance : public Component {
    struct Surface {
        RID primitive;
        RID material;
    };
    std::vector<Surface> surfaces;

   public:
    virtual void Draw() override;

    MeshInstance() {}
    ~MeshInstance() {}
};

}  // namespace Gauge