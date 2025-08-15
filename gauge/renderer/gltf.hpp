#pragma once

#include <gauge/common.hpp>
#include <gauge/renderer/common.hpp>
#include <gauge/renderer/texture.hpp>

#include <sys/types.h>
#include <string>
#include <unordered_map>

namespace Gauge {

struct glTF {
    std::unordered_map<std::string, CPUMesh> meshes;
    std::unordered_map<std::string, Texture> textures;

    static Result<glTF> FromFile(const std::string& p_path);
};

}  // namespace Gauge