#include "config.hpp"
#include <yaml-cpp/yaml.h>

using namespace Gauge;

ProjectSettings Gauge::load_project_settings(const std::string p_path) {
    YAML::Node config = YAML::LoadFile(p_path);
    return ProjectSettings {
        .name = config["name"].as<std::string>(),
        .description =config["description"].as<std::string>()
    };
}