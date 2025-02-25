#include "config.hpp"
#include <yaml-cpp/yaml.h>

using namespace Gauge;

ProjectSettings Gauge::load_project_settings(const std::string p_path) {
    // TODO: Load yaml file
    return ProjectSettings {
        .name = "Sandbox",
        .description = "Bla"
    };
}