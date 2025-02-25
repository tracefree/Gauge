#include <gauge.hpp>

int main() {
    Gauge::ProjectSettings project_settings = Gauge::load_project_settings("project.yaml");
    Gauge::App game;
    game.name = project_settings.name;

    game.run();
}
