#include "app.hpp"

#include <print>
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

using namespace Gauge;

App* gApp {nullptr};

void App::initialize() {
    gApp = this;
    
    const char* c_name = name.c_str();
    SDL_SetAppMetadata(c_name, "0.1", c_name);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("SDL could not initialize! SDL error: %s\n", SDL_GetError());
    }

    Renderer::initialize();
}

void App::quit() {
    quit_requested = true;
}