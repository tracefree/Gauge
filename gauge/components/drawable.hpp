#pragma once

namespace Gauge {

struct IDrawable {
    virtual void Draw() const = 0;
};

}  // namespace Gauge