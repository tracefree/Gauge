#include "shader_module.hpp"

#include <gauge/common.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <volk.h>

using namespace Gauge;

Result<ShaderModule>
ShaderModule::FromFile(const VulkanContext& ctx, std::string p_file_name) {
    auto shader_code_result = FileSystem::ReadFile(p_file_name);
    CHECK_RET(shader_code_result);
    return ShaderModule::FromCode(ctx, shader_code_result.value());
}

Result<ShaderModule>
ShaderModule::FromCode(const VulkanContext& ctx, std::vector<char> p_code) {
    ShaderModule shader_module{};
    VkShaderModuleCreateInfo shader_module_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = p_code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(p_code.data()),
    };
    VK_CHECK_RET(vkCreateShaderModule(ctx.device, &shader_module_info, nullptr, &shader_module.handle), "Could not create shader module");
    return shader_module;
}