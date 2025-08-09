#pragma once

#include <gauge/core/filesystem.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <vulkan/vulkan_core.h>
#include <expected>
#include <string>
#include <vector>

namespace Gauge {

struct ShaderModule {
    VkShaderModule handle{};

   public:
    static std::expected<ShaderModule, std::string> FromFile(const VulkanContext& ctx, std::string p_file_name);
    static std::expected<ShaderModule, std::string> FromCode(const VulkanContext& ctx, std::vector<char> p_code);
};

}  // namespace Gauge