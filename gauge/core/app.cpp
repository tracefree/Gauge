#include "app.hpp"

#include <SDL3/SDL_messagebox.h>
#include <chrono>
#include <gauge/renderer/vulkan/renderer_vulkan.hpp>

#include <memory>
#include <print>

#include <SDL3/SDL.h>
#include "thirdparty/tracy/public/common/TracySystem.hpp"

using namespace Gauge;

App* gApp{nullptr};

void App::Initialize() {
    gApp = this;

    start_time = std::chrono::steady_clock::now();

    // putenv((char*)"SDL_VIDEODRIVER=wayland");

    tracy::SetThreadName("main");

    const char* c_name = name.c_str();
    SDL_SetAppMetadata(c_name, "0.1", c_name);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("SDL could not initialize! SDL error: {}\n",
                     SDL_GetError());
    }

    renderer = std::make_unique<RendererVulkan>();
    renderer->SetFramesInFlight(3);
}

void App::Quit() {
    quit_requested = true;
}