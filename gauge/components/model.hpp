#pragma once

#include <gauge/components/component.hpp>

#include <string>

namespace Gauge {

struct ModelComponent final : public Component {
   public:
    std::string path;
    bool generate_collisions{};

    void Initialize() final override;

   public:
    static void StaticInitialize() {}
    COMPONENT_FACTORY_HEADER(ModelComponent)
};

}  // namespace Gauge