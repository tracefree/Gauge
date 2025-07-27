#pragma once

#include <SDL3/SDL_video.h>
#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include <gauge/renderer/renderer.hpp>

#include "thirdparty/volk/volk.h"

#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

namespace Gauge {
struct RendererVulkan : public Renderer {
   private:
    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkSemaphore swapchain_release_semaphore{};
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

    vkb::Instance instance;
    vkb::PhysicalDevice physical_device;
    vkb::Device device;
    VkQueue graphics_queue;

    std::vector<FrameData> frames_in_flight;
    unsigned long int current_frame_index;
    // VkSemaphore graphics_semaphore;
    // uint64_t timeline_value = 0;
    // uint64_t graphics_wait_value = 0;
    // uint64_t graphics_signal_value = 0;

    vkb::InstanceDispatchTable instance_dispatch;
    vkb::DispatchTable dispatch;

    VkSurfaceKHR surface;
    SDL_Window* window = nullptr;

   public:
    vkb::Instance const* get_instance() const;

    std::expected<void, std::string> initialize(SDL_Window* p_sdl_window) override;
    void draw() override;
    std::expected<void, std::string> create_surface(SDL_Window* window) override;
    std::expected<VkCommandPool, std::string> create_command_pool() const;
    std::expected<VkCommandBuffer, std::string> create_command_buffer(VkCommandPool p_cmd_pool) const;
    std::expected<void, std::string> create_swapchain(bool recreate = false);
    void transition_image(VkCommandBuffer p_cmd, VkImage p_image, VkImageLayout p_current_layout, VkImageLayout p_target_layout, VkImageAspectFlags p_aspect_flags = VK_IMAGE_ASPECT_NONE) const;
    void set_debug_name(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;

   private:
    FrameData get_current_frame() const;
};
}  // namespace Gauge

#define VK_CHECK(result, message)                 \
    if (result != VK_SUCCESS) [[unlikely]] {      \
        std::println("Gauge Error: {}", message); \
    }

#define VK_CHECK_RET(result, return_value)                                                                    \
    if (result != VK_SUCCESS) [[unlikely]] {                                                                  \
        return std::unexpected(std::format("{}. Vulkan result: {}.", return_value, string_VkResult(result))); \
    }