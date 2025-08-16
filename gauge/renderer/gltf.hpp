#pragma once

#include <gauge/common.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/common.hpp>
#include <gauge/renderer/texture.hpp>

#include <sys/types.h>
#include <string>
#include <unordered_map>

namespace Gauge {

struct glTF {
    struct PBRMaterial {
        Vec4 albedo{1.0f};
        float roughness{};
        float metallic{};
        std::string texture_albedo;
        std::string texture_normal;
        std::string texture_metallic_roughness;
    };
    std::unordered_map<std::string, CPUMesh> meshes;
    std::unordered_map<std::string, Texture> textures;
    std::unordered_map<std::string, PBRMaterial> materials;

    static Result<glTF>
    FromFile(const std::string& p_path);
};

}  // namespace Gauge