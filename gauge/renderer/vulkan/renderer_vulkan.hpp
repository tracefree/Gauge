#pragma once

#include <gauge/core/math.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/pipeline.hpp>

#include <SDL3/SDL_video.h>
#include <cstdint>
#include <expected>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "thirdparty/tracy/public/tracy/TracyVulkan.hpp"

namespace Gauge {

struct RendererVulkan : public Renderer {
   private:
    VulkanContext ctx{};

    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};
#ifdef TRACY_ENABLE
        tracy::VkCtx* tracy_context{};
#endif
    };

    struct SwapchainData {
        vkb::Swapchain vkb_swapchain;
        VkSwapchainKHR handle{};
        VkFormat image_format{};
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        VkExtent2D extent{};
    } swapchain;

    struct PushConstants {
        glm::mat4 model_matrix;
        glm::mat4 view_projection;
        VkDeviceAddress vertex_buffer_address;
    };

    std::vector<VkSemaphore>
        swapchain_release_semaphores;

    std::vector<FrameData> frames_in_flight;
    uint64_t current_frame_index = 0;

    Pipeline graphics_pipeline{};

    VkSurfaceKHR surface{};
    SDL_Window* window = nullptr;

    struct WindowSize {
        uint width{};
        uint height{};
    } window_size;

    struct RenderState {
        std::vector<Viewport> viewports;
    } render_state{};

   public:
    std::expected<void, std::string>
    Initialize(SDL_Window* p_sdl_window) final override;
    void Draw() final override;
    void OnWindowResized(uint p_width, uint p_height) final override;

   private:
    FrameData GetCurrentFrame() const;
    vkb::Instance const* GetInstance() const;
    std::expected<VkCommandPool, std::string> CreateCommandPool() const;
    std::expected<VkCommandBuffer, std::string> CreateCommandBuffer(VkCommandPool p_cmd_pool) const;
    std::expected<Pipeline, std::string> CreateGraphicsPipeline(std::string p_name);
    std::expected<void, std::string> CreateSwapchain(bool recreate = false);
    std::expected<void, std::string> CreateFrameData();
    std::expected<void, std::string> InitializeImGui() const;
    void RecordCommands(CommandBufferVulkan* cmd, uint p_next_image_index) const;
    void RenderImGui(CommandBufferVulkan* cmd, uint p_next_image_index) const;
    void RenderViewport(CommandBufferVulkan* cmd, const Viewport& p_viewport, uint p_next_image_index) const;
    void SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;
};
}  // namespace Gauge
