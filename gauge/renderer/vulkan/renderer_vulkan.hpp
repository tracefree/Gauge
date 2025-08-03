#pragma once

#include <gauge/core/math.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>
#include <gauge/renderer/vulkan/pipeline.hpp>

#include <SDL3/SDL_video.h>
#include <cstdint>
#include <expected>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "thirdparty/tracy/public/tracy/TracyVulkan.hpp"
#include "thirdparty/vk-bootstrap/src/VkBootstrap.h"

namespace Gauge {

struct RendererVulkan : public Renderer {
   private:
    struct Context {
        vkb::Instance instance{};
        vkb::PhysicalDevice physical_device{};
        vkb::Device device{};
        VkQueue graphics_queue{};
    } context;

    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};
#ifdef TRACY_ENABLE
        tracy::VkCtx* tracy_context;
#endif
    };

    struct Window {
        struct Size {
            uint width{};
            uint height{};
        } size;

        struct SwapchainData {
            vkb::Swapchain vkb_swapchain;
            VkSwapchainKHR handle;
            VkFormat image_format;
            std::vector<VkImage> images;
            std::vector<VkImageView> image_views;
            VkExtent2D extent;
        } swapchain;

        uint id{};

        std::vector<VkSemaphore> swapchain_release_semaphores;
        std::vector<FrameData> frames_in_flight;
        uint64_t current_frame_index;

        VkSurfaceKHR surface;
        SDL_Window* window = nullptr;
    };

    struct PushConstants {
        glm::vec4 color;
        //    glm::mat4 model_matrix;
        //    glm::mat4 view_projection;
        //    VkDeviceAddress vertex_buffer_address;
    };

    struct RenderState {
        std::unordered_map<SDL_WindowID, RendererVulkan::Window> windows;
        std::vector<Viewport> viewports;
        // Meshes, textures, materials, lights, scene data...
    } render_state;

    Pipeline graphics_pipeline;

   public:
    std::expected<void, std::string>
    Initialize(SDL_Window* p_sdl_window) final override;

    void OnWindowResized(SDL_WindowID p_window_id, uint p_width, uint p_height) final override;
    void Draw() final override;
    void RecordCommands(CommandBufferVulkan* cmd, uint p_next_image_index) const;
    void RenderImGui(CommandBufferVulkan* cmd, uint p_next_image_index) const;

    vkb::Instance const* GetInstance() const;
    std::expected<VkCommandPool, std::string> CreateCommandPool() const;
    std::expected<VkCommandBuffer, std::string> CreateCommandBuffer(VkCommandPool p_cmd_pool) const;
    void SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;

   private:
    std::expected<VkShaderModule, std::string> CreateShaderModule(const std::vector<char>& p_code) const;
    std::expected<Pipeline, std::string> CreateGraphicsPipeline(std::string p_name);
    std::expected<void, std::string> CreateSwapchain(bool recreate = false);
    std::expected<void, std::string> CreateFrameData();
    FrameData GetCurrentFrame() const;
    std::expected<void, std::string> InitializeImGui() const;
};
}  // namespace Gauge
