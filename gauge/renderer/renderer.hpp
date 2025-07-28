#pragma once

#include <SDL3/SDL_video.h>
#include <expected>

namespace Gauge {
struct Renderer {
   protected:
    bool initialized = false;
    uint max_frames_in_flight = 3;

   public:
    virtual std::expected<void, std::string> initialize(SDL_Window* p_sdl_window) = 0;
    virtual void draw() = 0;
    virtual std::expected<void, std::string> create_surface(SDL_Window* window) = 0;
    virtual void on_window_resized() {};

    Renderer() = default;
    virtual ~Renderer() = default;

    uint get_frames_in_flight() const;
    void set_frames_in_flight(uint p_max_frames_in_flight);
};
}  // namespace Gauge