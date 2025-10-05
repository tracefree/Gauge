#pragma once

#include <gauge/common.hpp>
#include <gauge/core/handle.hpp>
#include <gauge/math/common.hpp>
#include <gauge/math/transform.hpp>
#include <gauge/renderer/aabb.hpp>
#include <gauge/renderer/common.hpp>
#include <gauge/renderer/texture.hpp>
#include <gauge/renderer/vulkan/common.hpp>
#include <gauge/scene/node.hpp>

#include <sys/types.h>
#include <filesystem>
#include <optional>
#include <string>
#include "gauge/core/string_id.hpp"

namespace fastgltf {
class Asset;
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
        Handle<GPUImage> handle{};
        std::string name;
        Gauge::Texture* data{};
    };

    struct Material {
        Handle<GPUMaterial> handle{};
        std::string name;
        Vec4 albedo{1.0f};
        float roughness{};
        float metallic{};
        std::optional<uint> texture_albedo_index;
        std::optional<uint> texture_normal_index;
        std::optional<uint> texture_metallic_roughness_index;
        StringID shader_id;
    };

    struct Primitive {
        Handle<GPUMesh> handle{};
        std::vector<Vertex> vertices;
        std::vector<uint> indices;
        std::optional<uint> material_index;
        AABB aabb{};
    };

    struct Mesh {
        std::string name;
        std::vector<Primitive> primitives;
        AABB aabb{};
    };

    std::string name;
    std::vector<glTF::Node> nodes;
    std::vector<glTF::Texture> textures;
    std::vector<glTF::Material> materials;
    std::vector<glTF::Mesh> meshes;

   private:
    Result<> LoadNodes(const fastgltf::Asset& p_asset);
    Result<> LoadTextures(const fastgltf::Asset& p_asset, const std::filesystem::path& p_path = std::filesystem::path());
    Result<> LoadMaterials(const fastgltf::Asset& p_asset);
    Result<> LoadMeshes(const fastgltf::Asset& p_asset);

   public:
    static Result<glTF> FromFile(const std::string& p_path);
    Result<Ref<Gauge::Node>> CreateNode() const;

    // --- Resource interface ---
    static glTF Load(StringID p_id) {
        return FromFile(p_id).value();
    }
    void Unload() {
    }
};

}  // namespace Gauge