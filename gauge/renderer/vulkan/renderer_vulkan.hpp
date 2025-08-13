#pragma once

#include <functional>
#include <gauge/core/math.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <expected>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>
#include <vector>

#include "gauge/renderer/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "thirdparty/tracy/public/tracy/TracyVulkan.hpp"

namespace Gauge {

struct RendererVulkan : public Renderer {
   private:
    VulkanContext ctx{};

    struct PushConstants {
        Mat4 model_matrix;
        Mat4 view_projection;
        VkDeviceAddress vertex_buffer_address;
    };

    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};
        PushConstants push_constants{};
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

    struct Viewport {
        ViewportSettings settings{};
        GPUImage depth{};
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
        std::vector<Model> models;
        Vec3 camera_position{};
    } render_state{};

    struct ImmediateCommand {
        VkCommandPool pool{};
        VkCommandBuffer buffer{};
        VkFence fence{};
    } immediate_command{};

   public:
    std::expected<void, std::string>
    Initialize(SDL_Window* p_sdl_window) final override;
    void Draw() final override;
    void OnWindowResized(uint p_width, uint p_height) final override;
    void OnViewportResized(Viewport& p_viewport, uint p_width, uint p_height) const;

   private:
    FrameData const& GetCurrentFrame() const;
    FrameData& GetCurrentFrame();

    std::expected<VkCommandPool, std::string> CreateCommandPool() const;
    std::expected<VkCommandBuffer, std::string> CreateCommandBuffer(VkCommandPool p_cmd_pool) const;
    std::expected<Pipeline, std::string> CreateGraphicsPipeline(std::string p_name);
    std::expected<void, std::string> CreateSwapchain(bool recreate = false);
    std::expected<void, std::string> CreateFrameData();
    std::expected<void, std::string> CreateImmadiateCommand();
    std::expected<GPUImage, std::string> CreateDepthImage(const uint p_width, const uint p_height) const;
    std::expected<Viewport, std::string> CreateViewport(const ViewportSettings& p_settings) const;
    std::expected<BufferAllocation, std::string> CreateBuffer(size_t p_allocation_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const;

    void DestroyImage(GPUImage& p_image) const;

    std::expected<void, std::string> InitializeImGui() const;
    void RecordCommands(CommandBufferVulkan* cmd, uint p_next_image_index);
    void RenderImGui(CommandBufferVulkan* cmd, uint p_next_image_index) const;
    void RenderViewport(CommandBufferVulkan* cmd, const Viewport& p_viewport, uint p_next_image_index);
    void SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;

    std::expected<void, std::string> ImmediateSubmit(std::function<void(VkCommandBuffer p_cmd)>&& function) const;
    std::expected<GPUMesh, std::string> UploadMeshToGPU(const CPUMesh& mesh) const;
};
}  // namespace Gauge
