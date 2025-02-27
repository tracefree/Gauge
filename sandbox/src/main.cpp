#include <gauge/core/app.hpp>
#include <gauge/core/config.hpp>
#include "game.hpp"


int main() {
    Gauge::ProjectSettings project_settings = Gauge::load_project_settings("project.yaml");
    Sandbox::Game game;
    game.name = project_settings.name;
    game.initialize();
    return game.run();
}