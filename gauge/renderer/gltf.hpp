#pragma once

#include <expected>
#include <string>

#include "glm/ext/vector_float3.hpp"

namespace Gauge {

struct glTF {
    struct Vertex {
        glm::vec3 position;
    };

    static std::expected<glTF, std::string> FromFile(const std::string& p_path);
};

}  // namespace Gauge