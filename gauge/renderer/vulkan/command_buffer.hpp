#pragma once

#include <gauge/renderer/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <expected>
#include <string>

#include <volk.h>

namespace Gauge {
struct CommandBufferVulkan final : public CommandBuffer {
   private:
    VkCommandBuffer cmd;

   public:
    std::expected<void, std::string> Begin() final override;
    std::expected<void, std::string> End() final override;

    void TransitionImage(VkImage p_image, VkImageLayout p_current_layout, VkImageLayout p_target_layout, VkImageAspectFlags p_aspect_flags = VK_IMAGE_ASPECT_NONE) const;
    VkCommandBuffer GetHandle();
    void BindPipeline(const Pipeline& p_pipeline);

    CommandBufferVulkan(VkCommandBuffer cmd);
};
}  // namespace Gauge