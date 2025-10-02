#pragma once

#include <gauge/common.hpp>
#include <gauge/core/pool.hpp>
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

#include "gauge/core/handle.hpp"
#include "gauge/math/common.hpp"
#include "gauge/renderer/gltf.hpp"
#include "thirdparty/tracy/public/tracy/TracyVulkan.hpp"

namespace Gauge {

using RenderCallback = bool (*)(const VulkanContext& ctx, const CommandBufferVulkan& cmd);

struct RendererVulkan : public Renderer {
   public:
    VulkanContext ctx{};

    struct MousePosition {
        uint16_t x;
        uint16_t y;
    };

    struct PushConstants {
        Mat4 model_matrix;
        VkDeviceAddress vertex_buffer_address;
        uint sampler;
        uint material_index;
        uint camera_index;
        uint scene_index;
        MousePosition mouse_position;
        uint node_handle;
    };

    struct PCS_AABB {
        Vec3 position;
        uint camera_id;
        Vec3 extent;
        float _padding1;
    };

    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};
        PushConstants push_constants{};
        PCS_AABB pcs_aabb{};

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

        Vec3 camera_position{};
        float camera_yaw{};
        float camera_pitch{};
        float field_of_view = 70.0f;

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
    Pipeline aabb_pipeline{};

    // Updated when loading assets: Textures, samplers, materials...
    struct GlobalDescriptor {
        VkDescriptorPool pool{};
        VkDescriptorSetLayout layout{};
        DescriptorSet set{};
    } global_descriptor;

    VkSurfaceKHR surface{};

    struct WindowSize {
        uint width{};
        uint height{};
    } window_size;

    struct RenderState {
        std::vector<Viewport> viewports;
        std::vector<Model> models;
        std::vector<RenderCallback> render_callbacks;
        std::vector<Mat4> camera_views;
        std::vector<Mat4> camera_projections;
        std::vector<Mat4> camera_view_projections;
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
        Pool<GPUMesh> meshes;
        Pool<GPUImage> textures;
        Pool<GPUMaterial> materials{};

        GPUBuffer materials_buffer;
        GPUBuffer readback_buffer;

        Handle<GPUImage> texture_white;
        Handle<GPUImage> texture_black;
        Handle<GPUImage> texture_normal;
        Handle<GPUImage> texture_missing;
    } resources;

    VmaPool external_pool{};

    bool linear = true;
    bool offscreen = false;
    std::weak_ptr<Node> hovered_node;

   public:
    Result<> Initialize(void (*p_create_surface)(VkInstance p_instance, VkSurfaceKHR* r_surface), bool p_offscreen = false) final override;
    void Draw() final override;
    void DrawOffscreen() final override;

    virtual Handle<GPUMesh> CreateMesh(std::vector<Vertex> p_vertices, std::vector<uint> p_indices) final override;
    virtual void DestroyMesh(Handle<GPUMesh> p_handle) final override;

    virtual Handle<GPUImage> CreateTexture(const Texture& p_texture) final override;
    virtual void DestroyTexture(Handle<GPUImage> p_handle) final override;

    virtual Handle<GPUMaterial> CreateMaterial(const GPUMaterial& p_material) final override;
    virtual void DestroyMaterial(Handle<GPUMaterial> p_handle) final override;

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
    Result<Pipeline> CreateAABBPipeline(std::string p_name);
    Result<> CreateSwapchain(bool recreate = false);
    Result<> CreateFrameData();
    Result<> CreateImmadiateCommand();
    Result<GPUImage> CreateImage(
        VkExtent3D p_size,
        VkFormat p_format,
        VkImageUsageFlags p_usage,
        bool p_mipmapped = false,
        VkSampleCountFlagBits p_sample_count = VK_SAMPLE_COUNT_1_BIT,
        VkImageAspectFlagBits p_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
        bool p_exported = false) const;
    Result<GPUImage> CreateDepthImage(const uint p_width, const uint p_height, VkSampleCountFlagBits p_sample_count = VK_SAMPLE_COUNT_1_BIT) const;
    Result<Viewport> CreateViewport(const ViewportSettings& p_settings) const;
    Result<GPUBuffer> CreateBuffer(size_t p_allocation_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const;

    void DestroyImage(GPUImage& p_image) const;

    Result<> InitializeGlobalResources();
    void RecordCommands(const CommandBufferVulkan& cmd, uint p_next_image_index);
    void RenderImGui(CommandBufferVulkan* cmd, uint p_next_image_index) const;
    void RenderViewport(const CommandBufferVulkan& cmd, const Viewport& p_viewport, uint p_next_image_index);
    void SetDebugName(uint64_t p_handle, VkObjectType p_type, const std::string& p_name) const;

    Result<> ViewportCreateImages(Viewport& p_viewport) const;
    void ViewportDestroyImages(Viewport& p_viewport) const;

    void ViewportSetCameraView(uint p_viewport_id, const Mat4& p_view) final override;
    void ViewportSetCameraPosition(uint p_viewport_id, const Vec3& p_position) final override;
    void ViewportMoveCamera(uint p_viewport_id, const Vec3& p_offset) final override;
    void ViewportRotateCamera(uint p_viewport_id, float p_yaw, float p_pitch) final override;
    Quaternion ViewportGetCameraRotation(uint p_viewport_id) final override;

    Result<> ImmediateSubmit(std::function<void(CommandBufferVulkan p_cmd)>&& function) const;

    Result<GPUMesh> UploadMeshToGPU(const std::vector<Vertex>& p_vertices, const std::vector<uint>& p_indices) const;
    Result<GPUMesh> UploadMeshToGPU(const CPUMesh& mesh) const;
    Result<GPUMesh> UploadMeshToGPU(const glTF::Primitive& primitive) const;
    Result<GPUImage> UploadTextureToGPU(const Texture& p_texture) const;

    static VkSampleCountFlagBits SampleCountFromMSAA(MSAA p_msaa);

    std::weak_ptr<Node> GetHoveredNode() final override;
};
}  // namespace Gauge
