#pragma once

#include <gauge/components/component.hpp>

namespace Gauge {
void RegisterTypes();
void RegisterShaders();
void RegisterMaterialTypes();
void InitializeSystems();
void FinalizeSystems();
}  // namespace Gauge
