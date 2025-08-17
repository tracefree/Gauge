#pragma once

#include <string>
namespace Gauge {

struct Component {
    std::string name;
    bool active = true;
    bool visible = true;

    virtual void Update() {}
    virtual void Draw() {}
};

}  // namespace Gauge