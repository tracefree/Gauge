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

        std::string title {"New Window"};
        Resolution resolution;
        bool fullscreen;

    private:
        SDL_Window* sdl_window {nullptr};
        bool visible = true;

    public:
        void initialize(bool p_create_hidden = false);
        void show();
        void hide();
        SDL_Window* get_sdl_window() const;

        Window(
            const std::string& p_title = "New Window",
            Resolution p_resolution = Resolution {.width = 1920, .height = 1080},
            bool p_create_hidden = false
        ) {
            title = p_title;
            resolution = p_resolution;
        }

        ~Window() {
            if (sdl_window) {
                SDL_DestroyWindow(sdl_window);
            }
        }
    };
}