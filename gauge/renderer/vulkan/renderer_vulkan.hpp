#pragma once

#include <expected>
#include <vector>

#include <gauge/renderer/renderer.hpp>

#include "thirdparty/volk/volk.h"

#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

namespace Gauge {
struct RendererVulkan : public Renderer {
   private:
    struct FrameData {
        VkCommandPool cmd_pool;
        VkCommandBuffer cmd;
    };

    vkb::Instance instance;
    vkb::PhysicalDevice physical_device;
    vkb::Device device;
    VkQueue graphics_queue;

    std::vector<FrameData> frames_in_flight;
    unsigned long int current_frame_index;

    vkb::InstanceDispatchTable instance_dispatch;
    vkb::DispatchTable dispatch;

    VkSwapchainKHR swapchain;
    VkFormat swapchain_image_format;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkExtent2D swapchain_extent;
    VkSurfaceKHR surface;

   public:
    vkb::Instance const* get_instance() const;

    bool initialize(SDL_Window* p_sdl_window) override;
    void draw() override;
    std::expected<void, std::string> create_surface(SDL_Window* window) override;

   private:
    std::expected<VkCommandPool, std::string> create_command_pool() const;
    std::expected<VkCommandBuffer, std::string> create_command_buffer(VkCommandPool p_cmd_pool) const;
    FrameData get_current_frame() const;
};
}  // namespace Gauge

#define VK_CHECK(result, message)                 \
    if (result != VK_SUCCESS) [[unlikely]] {      \
        std::println("Gauge Error: {}", message); \
    }

#define VK_CHECK_RET(result, return_value)    \
    if (result != VK_SUCCESS) [[unlikely]] {  \
        return std::unexpected(return_value); \
    }