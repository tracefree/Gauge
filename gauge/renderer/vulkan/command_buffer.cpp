#include "command_buffer.hpp"

#include <gauge/renderer/vulkan/common.hpp>

#include <expected>

using namespace Gauge;

std::expected<void, std::string>
CommandBufferVulkan::Begin() {
    const VkCommandBufferBeginInfo cmd_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    VK_CHECK_RET(vkBeginCommandBuffer(cmd, &cmd_begin_info),
                 "Could not begin command buffer");

    return {};
}

std::expected<void, std::string>
CommandBufferVulkan::End() {
    VK_CHECK_RET(vkEndCommandBuffer(cmd),
                 "Could not end command buffer");

    return {};
}

CommandBufferVulkan::CommandBufferVulkan(VkCommandBuffer p_cmd) {
    cmd = p_cmd;
}

void CommandBufferVulkan::transition_image(VkImage p_image, VkImageLayout p_current_layout, VkImageLayout p_target_layout, VkImageAspectFlags p_aspect_flags) const {
    VkImageAspectFlags aspect_mask;
    if (p_aspect_flags == VK_IMAGE_ASPECT_NONE) {
        aspect_mask = (p_target_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || p_current_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                          ? VK_IMAGE_ASPECT_DEPTH_BIT
                          : VK_IMAGE_ASPECT_COLOR_BIT;
    } else {
        aspect_mask = p_aspect_flags;
    }

    VkAccessFlagBits2 src_access_mask{};
    VkPipelineStageFlags2 src_stage_mask{};
    switch (p_current_layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            src_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
            src_stage_mask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            src_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            break;
        default:;
    }

    VkAccessFlagBits2 dst_access_mask{};
    VkPipelineStageFlags2 dst_stage_mask{};
    switch (p_target_layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            dst_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
            dst_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            dst_stage_mask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            break;
        default:;
    }

    VkImageMemoryBarrier2 image_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = p_current_layout,
        .newLayout = p_target_layout,
        // srcQueueFamilyIndex
        // dstQueueFamilyIndex
        .image = p_image,
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };

    VkDependencyInfo dependency_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,  // ?
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier,
    };

    vkCmdPipelineBarrier2(cmd, &dependency_info);
}