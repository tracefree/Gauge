#pragma once

#include <gauge/common.hpp>
#include <gauge/renderer/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <volk.h>

namespace Gauge {
struct CommandBufferVulkan final : public CommandBuffer {
   private:
    VkCommandBuffer cmd;

   public:
    Result<> Begin() final override;
    Result<> End() final override;

    void TransitionImage(VkImage p_image, VkImageLayout p_current_layout, VkImageLayout p_target_layout, VkImageAspectFlags p_aspect_flags = VK_IMAGE_ASPECT_NONE) const;
    VkCommandBuffer GetHandle() const;
    void BindPipeline(const Pipeline& p_pipeline) const;

    CommandBufferVulkan(VkCommandBuffer cmd);
};
}  // namespace Gauge