#pragma once

#include <sys/types.h>
#include <expected>
#include <string>

#include <gauge/renderer/common.hpp>
#include <unordered_map>

namespace Gauge {

struct glTF {
    std::unordered_map<std::string, Mesh> meshes;

    static std::expected<glTF, std::string> FromFile(const std::string& p_path);
};

}  // namespace Gauge