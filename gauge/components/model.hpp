#pragma once

#include <gauge/components/component.hpp>

#include <string>

namespace Gauge {

struct ModelComponent : public Component {
   public:
    std::string path;

    void Initialize() final override;

   public:
    COMPONENT_FACTORY_HEADER(ModelComponent)
};

}  // namespace Gauge