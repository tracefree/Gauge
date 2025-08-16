#pragma once

#include <gauge/common.hpp>
#include <gauge/core/filesystem.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <volk.h>
#include <string>
#include <vector>

namespace Gauge {

struct ShaderModule {
    VkShaderModule handle{};

   public:
    static Result<ShaderModule> FromFile(const VulkanContext& ctx, std::string p_file_name);
    static Result<ShaderModule> FromCode(const VulkanContext& ctx, std::vector<char> p_code);
};

}  // namespace Gauge