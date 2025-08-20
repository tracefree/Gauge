#pragma once

#include <gauge/common.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/common.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/texture.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/descriptor.hpp>
#include <gauge/scene/scene_tree.hpp>

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "gauge/math/common.hpp"
#include "gauge/renderer/gltf.hpp"
#include "thirdparty/tracy/public/tracy/TracyVulkan.hpp"

namespace Gauge {

struct RendererVulkan : public Renderer {
   public:
    VulkanContext ctx{};

    struct PushConstants {
        Mat4 model_matrix;
        VkDeviceAddress vertex_buffer_address;
        uint sampler;
        uint material_index;
        uint camera_index;
        uint scene_index;
    };

    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};
        PushConstants push_constants{};

        // Updated every frame: Camera position, lights...
        DescriptorSet descriptor_set{};
        GPUBuffer uniform_buffer{};

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

        GPUImage color{};
        GPUImage depth{};

        GPUImage color_multisampled{};
        GPUImage depth_multisampled{};

        std::shared_ptr<SceneTree> scene_tree{};
    };

    const uint MAX_DESCRIPTOR_SETS = 16536;

    std::vector<VkSemaphore>
        swapchain_release_semaphores;

    VkDescriptorPool per_frame_pool{};
    std::vector<FrameData> frames_in_flight;
    uint64_t current_frame_index = 0;

    Pipeline graphics_pipeline{};

    // Updated when loading assets: Textures, samplers, materials...
    struct GlobalDescriptor {
        VkDescriptorPool pool{};
        VkDescriptorSetLayout layout{};
        DescriptorSet set{};
    } global_descriptor;

    VkSurfaceKHR surface{};
    SDL_Window* window = nullptr;

    struct WindowSize {
        uint width{};
        uint height{};
    } window_size;

    struct RenderState {
        std::vector<Viewport> viewports;
        std::vector<Model> models;
        Vec3 camera_position = Vec3(0.0f, 0.9f, 1.8f);
    } render_state{};

    struct ImmediateCommand {
        VkCommandPool pool{};
        VkCommandBuffer buffer{};
        VkFence fence{};
    } immediate_command{};

    struct Samplers {
        VkSampler linear{};
        VkSampler nearest{};
    } samplers;

    struct GlobalResources {
        RID texture_white;
        RID texture_black;
        RID texture_normal;
        RID texture_missing;

        std::vector<GPUMesh> meshes;
        std::vector<GPUImage> textures;
        std::vector<GPUMaterial> materials;

        GPUBuffer materials_buffer;
    } resources;

    bool linear = true;

   public:
    Result<> Initialize(SDL_Window* p_sdl_window) final override;
    void Draw() final override;

    virtual RID CreateMesh(std::vector<Vertex> p_vertices, std::vector<uint> p_indices) final override;
    virtual void DestroyMesh(RID p_rid) final override;

    virtual RID CreateTexture(const Texture& p_texture) final override;
    virtual void DestroyTexture(RID p_rid) final override;

    virtual RID CreateMaterial(const GPUMaterial& p_material) final override;
    virtual void DestroyMaterial(RID p_rid) final override;

    void OnWindowResized(uint p_width, uint p_height) final override;
    void OnViewportResized(Viewport& p_viewport, uint p_width, uint p_height) const;
    void OnShaderChanged() final override;

   public:
    FrameData const& GetCurrentFrame() const;
    FrameData& GetCurrentFrame();

    Result<VkCommandPool> CreateCommandPool() const;
    Result<VkCommandBuffer> CreateCommandBuffer(VkCommandPool p_cmd_pool) const;
    Result<VkDescriptorPool> CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& p_pool_sizes, VkDescriptorPoolCreateFlagBits p_flags, uint p_max_sets) const;
    Result<VkDescriptorSet> CreateDescriptorSet(VkDescriptorPool p_pool, VkDescriptorSetLayout p_layout) const;
    Result<VkSampler> CreateSampler(VkFilter p_filter_mode) const;
    Result<Pipeline> CreateGraphicsPipeline(std::string p_name);
    Result<> CreateSwapchain(bool recreate = false);
    Result<> CreateFrameData();
    Result<> CreateImmadiateCommand();
    Result<GPUImage> CreateImage(
        VkExtent3D p_size,
        VkFormat p_format,
        VkImageUsageFlags p_usage,
        bool p_mipmapped = false,
        VkSampleCountFlagBits p_sample_count = VK_SAMPLE_COUNT_1_BIT,
        VkImageAspectFlagBits p_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT) const;
    Result<GPUImage> CreateDepthImage(const uint p_width, const uint p_height, VkSampleCountFlagBits p_sample_count = VK_SAMPLE_COUNT_1_BIT) const;
    Result<Viewport> CreateViewport(const ViewportSettings& p_settings) const;
    Result<GPUBuffer> CreateBuffer(size_t p_allocation_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const;

    void DestroyImage(GPUImage& p_image) const;

    Result<> InitializeGlobalResources();
    void RecordCommands(CommandBufferVulkan* cmd, uint p_next_image_index);
    void RenderImGui(CommandBufferVulkan* cmd, uint p_next_image_index) const;
    void RenderViewport(CommandBufferVulkan* cmd, const Viewport& p_viewport, uint p_next_image_index);
    void SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;

    Result<> ViewportCreateImages(Viewport& p_viewport) const;
    void ViewportDestroyImages(Viewport& p_viewport) const;

    Result<> ImmediateSubmit(std::function<void(CommandBufferVulkan p_cmd)>&& function) const;

    Result<GPUMesh> UploadMeshToGPU(const std::vector<Vertex>& p_vertices, const std::vector<uint>& p_indices) const;
    Result<GPUMesh> UploadMeshToGPU(const CPUMesh& mesh) const;
    Result<GPUMesh> UploadMeshToGPU(const glTF::Primitive& primitive) const;
    Result<GPUImage> UploadTextureToGPU(const Texture& p_texture) const;
};
}  // namespace Gauge
