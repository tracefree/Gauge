#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vulkan/vulkan.h>

namespace Gauge {
    struct Window {
    public:
        std::string title {"New Window"};
        uint width {1280};
        uint height {720};

    private:
        SDL_Window* sdl_window {nullptr};
        VkSurfaceKHR surface;

    public:
        void initialize(bool p_create_hidden = false);
        void show();

        Window(const std::string& p_title = "New Window", uint p_width = 1280, uint p_height = 720, bool p_create_hidden = false) {
            title = p_title;
            width = p_width;
            height = p_height;
        }

        ~Window() {
            if (sdl_window) {
                SDL_DestroyWindow(sdl_window);
            }
        }
    };
}