#include "config.hpp"

#include <gauge/ui/window.hpp>

#include <expected>

#include <yaml-cpp/yaml.h>
#include "gauge/common.hpp"
#include "thirdparty/tracy/public/tracy/Tracy.hpp"

using namespace Gauge;

Result<ProjectSettings>
Gauge::LoadProjectSettings(const std::string p_path) {
    ZoneScoped;
    YAML::Node config = YAML::LoadFile(p_path);
    try {
        return ProjectSettings{
            .name = config["name"].as<std::string>(),
            .description = config["description"].as<std::string>(),
            .resolution =
                Window::Resolution{
                    .width = config["resolution"]["width"].as<uint>(),
                    .height = config["resolution"]["height"].as<uint>(),
                },
            .msaa_level = (MSAA)config["msaa"].as<uint>(),
            .fullscreen = config["fullscreen"].as<bool>(),
        };
    } catch (YAML::Exception& e) {
        return Error(std::format("YAML: {}", e.msg));
    }
}