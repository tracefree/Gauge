#include "window.hpp"

#include <SDL3/SDL_video.h>
#include <gauge/core/app.hpp>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>

#include <SDL3/SDL_vulkan.h>
#include <print>

using namespace Gauge;

extern App* gApp;

void Window::initialize(bool p_create_hidden) {
    SDL_PropertiesID window_props {SDL_CreateProperties()};
    SDL_SetNumberProperty(window_props,  SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, resolution.width);
    SDL_SetNumberProperty(window_props,  SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, resolution.height);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, fullscreen);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, p_create_hidden);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetStringProperty(window_props,  SDL_PROP_WINDOW_CREATE_TITLE_STRING, title.c_str());
    sdl_window = SDL_CreateWindowWithProperties(window_props);
    if (!sdl_window) {
        std::print("Window could not be created! SDL error: %s\n", SDL_GetError());
    }
}

void Window::show() {
    visible = true;
    SDL_ShowWindow(sdl_window);
}

void Window::hide() {
    visible = false;
    SDL_HideWindow(sdl_window);
}

SDL_Window* Window::get_sdl_window() const {
    return sdl_window;
}