#pragma once

#include <gauge/components/component.hpp>
#include <gauge/math/common.hpp>

namespace Gauge {

struct CharacterController : public Component {
    virtual void Initialize() final override;
    virtual void Update(float delta) final override;

    COMPONENT_FACTORY_HEADER(CharacterController);
};

}  // namespace Gauge