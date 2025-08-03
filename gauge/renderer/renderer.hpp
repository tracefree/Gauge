#pragma once

#include <expected>

#include <SDL3/SDL_video.h>

namespace Gauge {

struct Viewport {
    struct Position {
        float x{};
        float y{};
    } position;
    float width{};
    float height{};
};

struct Renderer {
   protected:
    bool initialized = false;
    uint max_frames_in_flight = 3;

   public:
    uint GetFramesInFlight() const;
    void SetFramesInFlight(uint p_max_frames_in_flight);

    virtual std::expected<void, std::string> Initialize(SDL_Window* p_sdl_window) = 0;
    virtual void Draw() = 0;
    virtual void OnWindowResized(SDL_WindowID p_window_id, uint p_width, uint p_height) {};

    Renderer() = default;
    virtual ~Renderer() = default;
};

}  // namespace Gauge