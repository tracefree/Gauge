#pragma once

#include <gauge/common.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/common.hpp>
#include <gauge/renderer/texture.hpp>

#include <sys/types.h>
#include <optional>
#include <string>
#include "gauge/math/transform.hpp"

namespace fastgltf {
struct Asset;
}

namespace Gauge {

struct glTF {
   public:
    struct Node {
        std::string name;
        Transform transform{};
        std::vector<uint> children;
        std::optional<uint> mesh;
    };

    struct Texture {
        std::string name;
        Gauge::Texture data{};
    };

    struct Material {
        std::string name;
        Vec4 albedo{1.0f};
        float roughness{};
        float metallic{};
        std::optional<uint> texture_albedo_index;
        std::optional<uint> texture_normal_index;
        std::optional<uint> texture_metallic_roughness_index;
    };

    struct Primitive {
        std::vector<Vertex> vertices;
        std::vector<uint> indices;
        std::optional<uint> material_index;
    };

    struct Mesh {
        std::string name;
        std::vector<Primitive> primitives;
    };

    std::vector<glTF::Node> nodes;
    std::vector<glTF::Texture> textures;
    std::vector<glTF::Material> materials;
    std::vector<glTF::Mesh> meshes;

   public:
    static Result<glTF> FromFile(const std::string& p_path);

   private:
    Result<> LoadNodes(const fastgltf::Asset& p_asset);
    Result<> LoadTextures(const fastgltf::Asset& p_asset);
    Result<> LoadMaterials(const fastgltf::Asset& p_asset);
    Result<> LoadMeshes(const fastgltf::Asset& p_asset);
};

}  // namespace Gauge