#include "game.hpp"

#include <print>
#include <iostream>

int Sandbox::Game::run() {
    std::println("Running {}", name);

    // Create window
    window = Window(name);
    window.initialize();

    // Main loop
    while (!quit_requested) {
        update();
    }

    // Exit
    SDL_Quit();
    return 0;
}

void Sandbox::Game::update() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quit();
        }
    }
}