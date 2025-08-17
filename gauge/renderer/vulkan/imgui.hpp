#pragma once

#include <gauge/common.hpp>

namespace Gauge {

struct RendererVulkan;

Result<> InitializeImGui(const RendererVulkan& p_renderer);

}  // namespace Gauge