#pragma once

#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>

#include <SDL3/SDL_video.h>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include <volk.h>
#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

namespace Gauge {
struct RendererVulkan : public Renderer {
   private:
    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};
    };

    struct SwapchainData {
        vkb::Swapchain vkb_swapchain;
        VkSwapchainKHR handle;
        VkFormat image_format;
        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        VkExtent2D extent;
    } swapchain;

    std::vector<VkSemaphore> swapchain_release_semaphores;

    vkb::Instance instance;
    vkb::PhysicalDevice physical_device;
    vkb::Device device;
    VkQueue graphics_queue;

    std::vector<FrameData> frames_in_flight;
    uint64_t current_frame_index;

    VkSurfaceKHR surface;
    SDL_Window* window = nullptr;

   public:
    std::expected<void, std::string> Initialize(SDL_Window* p_sdl_window) final override;
    void OnWindowResized() final override;
    void Draw() final override;
    void RecordCommands(CommandBufferVulkan* cmd, uint p_next_image_index);

    vkb::Instance const* GetInstance() const;
    std::expected<VkCommandPool, std::string> CreateCommandPool() const;
    std::expected<VkCommandBuffer, std::string> CreateCommandBuffer(VkCommandPool p_cmd_pool) const;
    void SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;

   private:
    std::expected<void, std::string> CreateSwapchain(bool recreate = false);
    std::expected<void, std::string> CreateFrameData();
    FrameData GetCurrentFrame() const;
};
}  // namespace Gauge
