#pragma once

#include <string>
#include <gauge/ui/window.hpp>

namespace Gauge {
    enum class MSAA {
        OFF = 0,
        x2 = 2,
        x4 = 4,
        x8 = 8,
        x16 = 16,
    };

    struct ProjectSettings {
        std::string name;
        std::string description;
        MSAA msaa_level = MSAA::OFF;
        Window::Resolution resolution = Window::Resolution {.width = 1920, .height = 1080};
        bool fullscreen = false;
    };

    ProjectSettings load_project_settings(const std::string p_path);
}