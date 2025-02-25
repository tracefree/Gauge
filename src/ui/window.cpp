#include "window.hpp"

#include <print>

using namespace Gauge;

void Window::initialize(bool p_create_hidden) {
    SDL_PropertiesID window_props {SDL_CreateProperties()};
    SDL_SetNumberProperty(window_props,  SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(window_props,  SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, p_create_hidden);
    SDL_SetStringProperty(window_props,  SDL_PROP_WINDOW_CREATE_TITLE_STRING, title.c_str());
    auto sdl_window = SDL_CreateWindowWithProperties(window_props);
    if (sdl_window == nullptr) {
        std::print("Window could not be created! SDL error: %s\n", SDL_GetError());
    }
}

void Window::show() {
    SDL_ShowWindow(sdl_window);
}