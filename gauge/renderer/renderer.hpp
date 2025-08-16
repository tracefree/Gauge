#pragma once

#include <gauge/common.hpp>
#include <gauge/renderer/common.hpp>
#include "gauge/math/transform.hpp"
#include "gauge/renderer/texture.hpp"

#include <SDL3/SDL_video.h>

namespace Gauge {

struct ViewportSettings {
    struct Position {
        float x{};
        float y{};
    } position;
    float width{};
    float height{};

    bool fill_window{};
    bool use_swapchain{};
    bool use_depth{};
};

struct Renderer {
   public:
    struct DrawObject {
        RID primitive;
        RID material;
        Transform transform;
    };

    std::vector<DrawObject> draw_objects;

   protected:
    bool initialized = false;
    uint max_frames_in_flight = 3;

   public:
    uint GetFramesInFlight() const;
    void SetFramesInFlight(uint p_max_frames_in_flight);

    virtual Result<> Initialize(SDL_Window* p_sdl_window) = 0;
    virtual void Draw() = 0;
    virtual void OnWindowResized(uint p_width, uint p_height) {};
    virtual void OnMouseMoved(float p_position_x, float p_position_y) {};

    virtual RID CreateMesh(std::vector<Vertex> p_vertices, std::vector<uint> p_indices) = 0;
    virtual void DestroyMesh(RID p_rid) = 0;

    virtual RID CreateTexture(const Texture& p_texture) = 0;
    virtual void DestroyTexture(RID p_rid) = 0;

    virtual RID CreateMaterial(const GPUMaterial& p_material) = 0;
    virtual void DestroyMaterial(RID p_rid) = 0;

    Renderer() = default;
    virtual ~Renderer() = default;
};

}  // namespace Gauge