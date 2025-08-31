#pragma once

#include <SDL3/SDL.h>
#include <string>

namespace Gauge {
struct Window {
   public:
    struct Resolution {
        uint width;
        uint height;
    };

    std::string title{"New Window"};
    Resolution resolution;
    bool fullscreen;
    SDL_Window* sdl_window{nullptr};
  
   private:  
    bool visible = true;

   public:
    void Initialize(bool p_create_hidden = false);
    void Show();
    void Hide();
    SDL_Window* GetSDLHandle() const;

    Window(
        const std::string& p_title = "New Window",
        Resolution p_resolution = Resolution{.width = 1920, .height = 1080},
        bool p_create_hidden = false) {
        title = p_title;
        resolution = p_resolution;
    }

    ~Window() {
        if (sdl_window) {
            SDL_DestroyWindow(sdl_window);
        }
    }
};
}  // namespace Gauge