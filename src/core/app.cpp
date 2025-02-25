#include "app.hpp"

#include <print>
#include <SDL3/SDL.h>

using namespace Gauge;

void App::initialize() {
    SDL_SetAppMetadata(name.c_str(), "0.1", name.c_str());
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("SDL could not initialize! SDL error: %s\n", SDL_GetError());
    }
}

void App::quit() {
    quit_requested = true;
}