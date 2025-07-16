#include "app.hpp"

#include <SDL3/SDL_messagebox.h>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>

#include <memory>
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

    renderer = std::make_unique<RendererVulkan>();
    renderer->set_frames_in_flight(3);
}

void App::quit() {
    quit_requested = true;
}