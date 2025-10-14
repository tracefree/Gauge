#pragma once

#include <gauge/common.hpp>
#include <gauge/core/pool.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/common.hpp>
#include <gauge/renderer/renderer.hpp>
#include <gauge/renderer/shaders/shader.hpp>
#include <gauge/renderer/texture.hpp>
#include <gauge/renderer/vulkan/command_buffer.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/renderer/vulkan/descriptor.hpp>
#include <gauge/scene/scene_tree.hpp>

#include <SDL3/SDL_video.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <print>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "gauge/core/handle.hpp"
#include "gauge/math/common.hpp"
#include "gauge/renderer/gltf.hpp"
#include "thirdparty/tracy/public/tracy/TracyVulkan.hpp"

namespace Gauge {

class Shader;

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

    struct FrameData {
        VkCommandPool cmd_pool{};
        VkCommandBuffer cmd{};
        VkSemaphore swapchain_acquire_semaphore{};
        VkFence queue_submit_fence{};

        // Updated every frame: Camera position, lights...
        DescriptorSet descriptor_set{};
        GPUBuffer uniform_buffer{};
        GPUBuffer readback_buffer{};

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

    Pipeline aabb_pipeline{};

    std::unordered_map<std::type_index, Ref<Shader>> shaders;

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
        std::vector<GPUScene> scenes;
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

    struct MaterialTypeData {
        uint id;
        GPUBuffer buffer;
    };

    struct GlobalResources {
        Pool<GPUMesh> meshes;
        Pool<GPUImage> textures;
        Pool<GPUMaterial> materials;

        // Array of pointers to concrete material buffers
        std::vector<VkDeviceAddress> material_addresses;
        GPUBuffer materials_buffer;

        Handle<GPUImage> texture_white;
        Handle<GPUImage> texture_black;
        Handle<GPUImage> texture_normal;
        Handle<GPUImage> texture_missing;

        Handle<GPUMesh> debug_mesh_box;
        Handle<GPUMesh> debug_mesh_line;
    } resources;

    template <typename MaterialType>
    static Pool<MaterialType> materials;

    std::unordered_map<std::type_index, MaterialTypeData> material_types;
    uint registered_material_types = 0;

    VmaPool external_pool{};

    bool linear = true;
    bool offscreen = false;
    NodeHandle hovered_node;

   public:
    Result<> Initialize(void (*p_create_surface)(VkInstance p_instance, VkSurfaceKHR* r_surface), bool p_offscreen = false) final override;
    void Draw() final override;
    void DrawOffscreen() final override;

    virtual Handle<GPUMesh> CreateMesh(std::vector<Vertex> p_vertices, std::vector<uint> p_indices) final override;
    virtual Handle<GPUMesh> CreateMesh(std::vector<PositionVertex> p_vertices, std::vector<uint> p_indices) final override;

    virtual void DestroyMesh(Handle<GPUMesh> p_handle) final override;

    virtual Handle<GPUImage> CreateTexture(const Texture& p_texture) final override;
    virtual void DestroyTexture(Handle<GPUImage> p_handle) final override;

    template <typename MaterialType>
    Handle<GPUMaterial> CreateMaterial(const MaterialType& p_material);

    virtual void DestroyMaterial(Handle<GPU_PBRMaterial> p_handle) final override;

    void OnWindowResized(uint p_width, uint p_height) final override;
    void OnViewportResized(Viewport& p_viewport, uint p_width, uint p_height) const;
    void OnShaderChanged() final override;

    template <IsShader S>
    void RegisterShader() {
        auto shader = std::make_shared<S>();
        shader->Initialize(*this);
        shaders[std::type_index(typeid(S))] = shader;
    }

    template <IsShader S>
    Ref<S> GetShader() {
        return std::static_pointer_cast<S>(shaders[std::type_index(typeid(S))]);
    }

    template <typename MaterialType>
    inline void RegisterMaterialType() {
        MaterialTypeData material_type_data{
            .id = (uint)resources.material_addresses.size(),
        };
        material_type_data.buffer = CreateBuffer(
                                        sizeof(MaterialType) * 1000,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY)
                                        .value();
        const VkBufferDeviceAddressInfo buffer_address_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = material_type_data.buffer.handle,
        };
        VkDeviceAddress buffer_address = vkGetBufferDeviceAddress(ctx.device, &buffer_address_info);
        resources.material_addresses.push_back(buffer_address);
        material_types[std::type_index(typeid(MaterialType))] = material_type_data;

        // Staging
        const auto staging_buffer_result = CreateBuffer(
            sizeof(VkDeviceAddress),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_COPY);
        CHECK(staging_buffer_result);
        GPUBuffer staging_buffer = staging_buffer_result.value();

        void* data = staging_buffer.allocation.info.pMappedData;
        memcpy(data, &buffer_address, sizeof(VkDeviceAddress));

        ImmediateSubmit([&](CommandBufferVulkan cmd) {
            const VkBufferCopy buffer_copy{
                .dstOffset = (material_type_data.id) * sizeof(VkDeviceAddress),
                .size = sizeof(VkDeviceAddress),
            };
            vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, resources.materials_buffer.handle, 1, &buffer_copy);
        });

        vmaDestroyBuffer(ctx.allocator, staging_buffer.handle, staging_buffer.allocation.handle);
    }

    template <typename MaterialType>
    MaterialTypeData& GetMaterialTypeData() {
        return material_types[std::type_index(typeid(MaterialType))];
    }

   public:
    FrameData const& GetCurrentFrame() const;
    FrameData& GetCurrentFrame();

    Result<VkCommandPool> CreateCommandPool() const;
    Result<VkCommandBuffer> CreateCommandBuffer(VkCommandPool p_cmd_pool) const;
    Result<VkDescriptorPool> CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& p_pool_sizes, VkDescriptorPoolCreateFlagBits p_flags, uint p_max_sets) const;
    Result<VkDescriptorSet> CreateDescriptorSet(VkDescriptorPool p_pool, VkDescriptorSetLayout p_layout) const;
    Result<VkSampler> CreateSampler(VkFilter p_filter_mode) const;
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

    template <typename VertexType>
    Result<GPUMesh> UploadMeshToGPU(const std::vector<VertexType>& p_vertices, const std::vector<uint>& p_indices) const;

    Result<GPUMesh> UploadMeshToGPU(const CPUMesh& mesh) const;
    Result<GPUMesh> UploadMeshToGPU(const glTF::Primitive& primitive) const;
    Result<GPUImage> UploadTextureToGPU(const Texture& p_texture) const;

    static VkSampleCountFlagBits SampleCountFromMSAA(MSAA p_msaa);

    NodeHandle GetHoveredNode() final override;
};

template <typename MaterialType>
Handle<GPUMaterial> RendererVulkan::CreateMaterial(const MaterialType& p_material) {
    auto material_type_data = GetMaterialTypeData<MaterialType>();

    Handle<MaterialType> handle = materials<MaterialType>.Allocate(p_material);

    // Staging
    const auto staging_buffer_result = CreateBuffer(
        sizeof(MaterialType),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_COPY);
    CHECK(staging_buffer_result);
    GPUBuffer staging_buffer = staging_buffer_result.value();

    void* data = staging_buffer.allocation.info.pMappedData;
    memcpy(data, &p_material, sizeof(MaterialType));

    ImmediateSubmit([&](CommandBufferVulkan cmd) {
        const VkBufferCopy buffer_copy{
            .dstOffset = (handle.index) * sizeof(MaterialType),
            .size = sizeof(MaterialType),
        };
        vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, material_type_data.buffer.handle, 1, &buffer_copy);
    });

    vmaDestroyBuffer(ctx.allocator, staging_buffer.handle, staging_buffer.allocation.handle);
    return resources.materials.Allocate({
        .type = material_type_data.id,
        .id = handle.index,
    });
}

template <typename VertexType>
inline Result<GPUMesh>
RendererVulkan::UploadMeshToGPU(const std::vector<VertexType>& p_vertices, const std::vector<uint>& p_indices) const {
    GPUMesh gpu_mesh{};
    const uint vertex_buffer_size = p_vertices.size() * sizeof(VertexType);
    const size_t index_buffer_size = p_indices.size() * sizeof(uint);

    gpu_mesh.index_count = p_indices.size();

    // Vertices
    const auto vertex_buffer_result = CreateBuffer(
        vertex_buffer_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    CHECK_RET(vertex_buffer_result);
    gpu_mesh.vertex_buffer = vertex_buffer_result.value();
    const VkBufferDeviceAddressInfo vertex_buffer_address_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = gpu_mesh.vertex_buffer.handle,
    };
    gpu_mesh.vertex_buffer.address = vkGetBufferDeviceAddress(ctx.device, &vertex_buffer_address_info);

    // Indices
    const auto index_buffer_result = CreateBuffer(
        index_buffer_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    CHECK_RET(index_buffer_result);
    gpu_mesh.index_buffer = index_buffer_result.value();

    // Staging
    GPUBuffer staging_buffer{};
    const auto staging_buffer_result = CreateBuffer(
        (vertex_buffer_size + index_buffer_size),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_COPY);
    CHECK_RET(staging_buffer_result);
    staging_buffer = staging_buffer_result.value();

    void* data = staging_buffer.allocation.info.pMappedData;
    memcpy(data, p_vertices.data(), vertex_buffer_size);
    memcpy((char*)data + vertex_buffer_size, p_indices.data(), index_buffer_size);

    ImmediateSubmit([&](CommandBufferVulkan cmd) {
        const VkBufferCopy vertex_copy{
            .size = vertex_buffer_size,
        };
        vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, gpu_mesh.vertex_buffer.handle, 1, &vertex_copy);

        const VkBufferCopy index_copy{
            .srcOffset = vertex_buffer_size,
            .size = index_buffer_size,
        };
        vkCmdCopyBuffer(cmd.GetHandle(), staging_buffer.handle, gpu_mesh.index_buffer.handle, 1, &index_copy);
    });

    vmaDestroyBuffer(ctx.allocator, staging_buffer.handle, staging_buffer.allocation.handle);

    return gpu_mesh;
}

template <typename MaterialType>
Pool<MaterialType> RendererVulkan::materials;

}  // namespace Gauge