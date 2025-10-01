#pragma once

#include <gauge/common.hpp>
#include <gauge/core/config.hpp>
#include <gauge/core/pool.hpp>
#include <gauge/renderer/common.hpp>
#include "gauge/core/handle.hpp"
#include "gauge/math/common.hpp"
#include "gauge/math/transform.hpp"
#include "gauge/renderer/aabb.hpp"
#include "gauge/renderer/texture.hpp"
#include "gauge/renderer/vulkan/common.hpp"

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

namespace Gauge {

struct ViewportSettings {
    struct Position {
        float x{};
        float y{};
    } position;
    float width{};
    float height{};

    MSAA msaa = MSAA::x4;

    bool fill_window{};
    bool use_swapchain{};
    bool use_depth{};
    float render_scale = 1.0f;
};

struct Renderer {
   public:
    struct DrawObject {
        Handle<GPUMesh> primitive;
        Handle<GPUMaterial> material;
        Transform transform;
    };

    struct DrawAABB {
        AABB aabb;
        Transform transform;
    };

    std::vector<DrawObject> draw_objects;
    std::vector<DrawAABB> draw_aabbs;

   protected:
    bool initialized = false;
    uint max_frames_in_flight = 3;

   public:
    uint GetFramesInFlight() const;
    void SetFramesInFlight(uint p_max_frames_in_flight);

    virtual Result<> Initialize(void (*p_create_surface)(VkInstance p_instance, VkSurfaceKHR* r_surface), bool p_offscreen = false) = 0;
    virtual void Draw() = 0;
    virtual void DrawOffscreen() {};

    virtual void ViewportSetCameraView(uint p_viewport_id, const Mat4& p_view) = 0;
    virtual void ViewportSetCameraPosition(uint p_viewport_id, const Vec3& p_position) = 0;
    virtual void ViewportMoveCamera(uint p_viewport_id, const Vec3& p_offset) = 0;
    virtual void ViewportRotateCamera(uint p_viewport_id, float p_yaw, float p_pitch) = 0;
    virtual Quaternion ViewportGetCameraRotation(uint p_viewport_id) = 0;

    virtual void OnWindowResized(uint p_width, uint p_height) {};
    virtual void OnMouseMoved(float p_position_x, float p_position_y) {};
    virtual void OnShaderChanged() {};

    virtual Handle<GPUMesh> CreateMesh(std::vector<Vertex> p_vertices, std::vector<uint> p_indices) = 0;
    virtual void DestroyMesh(Handle<GPUMesh> p_handle) = 0;

    virtual Handle<GPUImage> CreateTexture(const Texture& p_texture) = 0;
    virtual void DestroyTexture(Handle<GPUImage> p_handle) = 0;

    virtual Handle<GPUMaterial> CreateMaterial(const GPUMaterial& p_material) = 0;
    virtual void DestroyMaterial(Handle<GPUMaterial> p_handle) = 0;

    Renderer() = default;
    virtual ~Renderer() = default;
};

}  // namespace Gauge