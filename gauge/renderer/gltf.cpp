#include "gltf.hpp"

#include <gauge/renderer/common.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <filesystem>
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"

using namespace Gauge;

namespace Attribute {
constexpr const char* POSITION = "POSTION";
constexpr const char* NORMAL = "NORMAL";
constexpr const char* UV = "TEXCOORD_0";
constexpr const char* TANGENT = "TANGENT";
}  // namespace Attribute

static void IterateIndices(fastgltf::Asset& asset, const fastgltf::Primitive& primitive, Mesh& mesh) {
    fastgltf::Accessor& accessor = asset.accessors[primitive.indicesAccessor.value()];
    mesh.indices.reserve(accessor.count);
    fastgltf::iterateAccessor<uint>(
        asset,
        accessor,
        [&](uint index) {
            mesh.indices.emplace_back(index);
        });
}

static void IteratePositions(fastgltf::Asset& asset, const fastgltf::Primitive& primitive, Mesh& mesh) {
    fastgltf::Accessor& accessor = asset.accessors[primitive.findAttribute(Attribute::POSITION)->accessorIndex];
    mesh.vertices.resize(accessor.count);
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset,
        accessor,
        [&](glm::vec3 position, size_t index) {
            mesh.vertices[index].position = position;
        });
}

static void IterateNormals(fastgltf::Asset& asset, const fastgltf::Primitive& primitive, Mesh& mesh) {
    fastgltf::Accessor& accessor = asset.accessors[primitive.findAttribute(Attribute::NORMAL)->accessorIndex];
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset,
        accessor,
        [&](glm::vec3 normal, size_t index) {
            mesh.vertices[index].normal = normal;
        });
}

static void IterateTangents(fastgltf::Asset& asset, const fastgltf::Primitive& primitive, Mesh& mesh) {
    fastgltf::Accessor& accessor = asset.accessors[primitive.findAttribute(Attribute::TANGENT)->accessorIndex];
    fastgltf::iterateAccessorWithIndex<glm::vec4>(
        asset,
        accessor,
        [&](glm::vec4 tangent, size_t index) {
            mesh.vertices[index].tangent = tangent;
        });
}

static void IterateUVs(fastgltf::Asset& asset, const fastgltf::Primitive& primitive, Mesh& mesh) {
    fastgltf::Accessor& accessor = asset.accessors[primitive.findAttribute(Attribute::UV)->accessorIndex];
    fastgltf::iterateAccessorWithIndex<glm::vec2>(
        asset,
        accessor,
        [&](glm::vec2 uv, size_t index) {
            mesh.vertices[index].uv_x = uv.x;
            mesh.vertices[index].uv_y = uv.y;
        });
}

std::expected<glTF, std::string>
glTF::FromFile(const std::string& p_path) {
    glTF gltf{};

    std::filesystem::path path(p_path);
    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None) {
        return std::unexpected(std::format("Could not load glTF file. fastgltf error: {}", fastgltf::getErrorMessage(data.error())));
    }

    auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
    if (asset.error() != fastgltf::Error::None) {
        return std::unexpected(std::format("Could not parse glTF file. fastgltf error: {}", fastgltf::getErrorMessage(asset.error())));
    }

    for (const auto& gltf_mesh : asset->meshes) {
        Mesh mesh{};

        for (auto&& primitive : gltf_mesh.primitives) {
            IterateIndices(asset.get(), primitive, mesh);
            IteratePositions(asset.get(), primitive, mesh);
            IterateNormals(asset.get(), primitive, mesh);
            //  IterateTangents(asset.get(), primitive, mesh);
            IterateUVs(asset.get(), primitive, mesh);
        }

        gltf.meshes[std::string(gltf_mesh.name)] = mesh;
    }

    return gltf;
}