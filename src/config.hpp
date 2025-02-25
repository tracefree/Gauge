#pragma once

#include <string>

namespace Gauge {
    struct ProjectSettings {
        std::string name;
        std::string description;
    };

    ProjectSettings load_project_settings(const std::string p_path);
}