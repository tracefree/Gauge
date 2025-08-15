#pragma once

#include <gauge/common.hpp>

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

    Renderer() = default;
    virtual ~Renderer() = default;
};

}  // namespace Gauge