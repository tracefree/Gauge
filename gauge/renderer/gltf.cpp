#include "gltf.hpp"

#include <cstdio>
#include <gauge/common.hpp>
#include <gauge/components/mesh_instance.hpp>
#include <gauge/renderer/common.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <filesystem>
#include <memory>
#include <print>
#include <variant>
#include "fastgltf/math.hpp"
#include "fastgltf/util.hpp"
#include "gauge/core/app.hpp"
#include "gauge/math/common.hpp"
#include "thirdparty/stb/stb_image.h"

using namespace Gauge;

extern App* gApp;

namespace Attribute {
constexpr const char* POSITION = "POSITION";
constexpr const char* NORMAL = "NORMAL";
constexpr const char* UV = "TEXCOORD_0";
constexpr const char* TANGENT = "TANGENT";
}  // namespace Attribute

static Vec4 Vec4FromFastGLTF(fastgltf::math::nvec4 p_vector) {
    return Vec4(
        p_vector[0],
        p_vector[1],
        p_vector[2],
        p_vector[3]);
}

static Vec3 Vec3FromFastGLTF(fastgltf::math::nvec3 p_vector) {
    return Vec3(
        p_vector[0],
        p_vector[1],
        p_vector[2]);
}

static Quaternion QuaternionFromFastGLTF(fastgltf::math::quat<float> p_quaternion) {
    return Quaternion(
        p_quaternion[3],
        p_quaternion[0],
        p_quaternion[1],
        p_quaternion[2]);
}

static void IterateIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    const fastgltf::Accessor& accessor = asset.accessors[fg_primitive.indicesAccessor.value()];
    primitive.indices.reserve(accessor.count);
    fastgltf::iterateAccessor<uint>(
        asset,
        accessor,
        [&](uint index) {
            primitive.indices.emplace_back(index);
        });
}

static void IteratePositions(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    auto attriubte = fg_primitive.findAttribute(Attribute::POSITION);
    assert(attriubte != fg_primitive.attributes.end());
    const fastgltf::Accessor& accessor = asset.accessors[attriubte->accessorIndex];
    primitive.vertices.resize(accessor.count);
    fastgltf::iterateAccessorWithIndex<Vec3>(
        asset,
        accessor,
        [&](Vec3 position, size_t index) {
            primitive.vertices[index].position = position;
        });
}

static void IterateNormals(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    auto attribute = fg_primitive.findAttribute(Attribute::NORMAL);
    if (attribute == fg_primitive.attributes.end())
        return;
    const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];
    fastgltf::iterateAccessorWithIndex<Vec3>(
        asset,
        accessor,
        [&](Vec3 normal, size_t index) {
            primitive.vertices[index].normal = normal;
        });
}

static void IterateTangents(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    auto attribute = fg_primitive.findAttribute(Attribute::TANGENT);
    if (attribute == fg_primitive.attributes.end()) {
        std::println("NONE");
        return;
    }
    const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];
    fastgltf::iterateAccessorWithIndex<Vec4>(
        asset,
        accessor,
        [&](Vec4 tangent, size_t index) {
            primitive.vertices[index].tangent = tangent;
        });
}

static void IterateUVs(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    auto attribute = fg_primitive.findAttribute(Attribute::UV);
    if (attribute == fg_primitive.attributes.end())
        return;
    const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];
    fastgltf::iterateAccessorWithIndex<Vec2>(
        asset,
        accessor,
        [&](Vec2 uv, size_t index) {
            primitive.vertices[index].uv_x = uv.x;
            primitive.vertices[index].uv_y = uv.y;
        });
}

Result<> glTF::LoadNodes(const fastgltf::Asset& p_asset) {
    nodes.resize(p_asset.nodes.size());
    for (uint i = 0; i < p_asset.nodes.size(); ++i) {
        const fastgltf::Node& fg_node = p_asset.nodes[i];
        glTF::Node& node = nodes[i];
        node.name = fg_node.name;
        fastgltf::TRS transform = std::get<fastgltf::TRS>(fg_node.transform);
        node.transform.position = Vec3FromFastGLTF(transform.translation);
        node.transform.rotation = QuaternionFromFastGLTF(transform.rotation);
        node.transform.scale = transform.scale[0];
        for (const auto& child_index : fg_node.children) {
            node.children.emplace_back(child_index);
        }
        if (fg_node.meshIndex.has_value()) {
            node.mesh = fg_node.meshIndex.value();
        }
    }
    return {};
}

Result<> glTF::LoadTextures(const fastgltf::Asset& p_asset) {
    textures.resize(p_asset.textures.size());
    for (uint i = 0; i < p_asset.textures.size(); ++i) {
        const fastgltf::Texture& fg_texture = p_asset.textures[i];
        glTF::Texture& texture = textures[i];
        auto fg_image = p_asset.images[fg_texture.imageIndex.value()];
        std::string err;
        std::visit(fastgltf::visitor{
                       [&](fastgltf::sources::URI& file_path) {
                           std::println("path {}", file_path.uri.c_str());
                           // TODO: Implement
                       },
                       [&](const fastgltf::sources::Array& array) {
                           std::println("Array!");
                       },
                       [&](fastgltf::sources::BufferView& view) {
                           auto& buffer_view = p_asset.bufferViews[view.bufferViewIndex];
                           auto& buffer = p_asset.buffers[buffer_view.bufferIndex];
                           std::visit(
                               fastgltf::visitor{
                                   [&](auto& argument) {
                                       err = "Could not load texture: Buffer type not implemented";
                                   },
                                   [&](const fastgltf::sources::Array& array) {
                                       int width, height, number_channels;
                                       texture.data.data = stbi_load_from_memory((stbi_uc*)array.bytes.data() + buffer_view.byteOffset, static_cast<int>(buffer_view.byteLength), &width, &height, &number_channels, 4);
                                       texture.data.width = (uint)width;
                                       texture.data.height = (uint)width;
                                       if (!texture.data.data) {
                                           err = "Could not load texture: Couldn't load data from memory";
                                           texture.data.data = nullptr;
                                       }
                                   },
                               },
                               buffer.data);
                       },
                       [&](auto& argument) {
                           err = "Could not load texture: data source not implemented";
                       },
                   },
                   fg_image.data);
        if (!err.empty()) {
            return Error(err);
        }
    }
    return {};
}

Result<> glTF::LoadMaterials(const fastgltf::Asset& p_asset) {
    materials.resize(p_asset.materials.size());
    for (uint i = 0; i < p_asset.materials.size(); ++i) {
        const fastgltf::Material& fg_material = p_asset.materials[i];
        glTF::Material& material = materials[i];
        material.name = fg_material.name;
        material.albedo = Vec4FromFastGLTF(fg_material.pbrData.baseColorFactor);
        material.metallic = fg_material.pbrData.metallicFactor;
        material.roughness = fg_material.pbrData.roughnessFactor;

        GPUMaterial gpu_material{
            .albedo = material.albedo,
            .metallic = material.metallic,
            .roughness = material.roughness,
        };

        if (fg_material.pbrData.baseColorTexture.has_value()) {
            material.texture_albedo_index = fg_material.pbrData.baseColorTexture->textureIndex;
            glTF::Texture& texture = textures[material.texture_albedo_index.value()];
            texture.data.use_srgb = true;
            if (texture.rid == 0) {
                texture.rid = gApp->renderer->CreateTexture(texture.data);
            }
            gpu_material.texture_albedo = texture.rid;
        }
        if (fg_material.normalTexture.has_value()) {
            material.texture_normal_index = fg_material.normalTexture->textureIndex;
            glTF::Texture& texture = textures[material.texture_normal_index.value()];
            texture.data.use_srgb = false;
            if (texture.rid == 0) {
                texture.rid = gApp->renderer->CreateTexture(texture.data);
            }
            gpu_material.texture_normal = textures[material.texture_normal_index.value()].rid;
        }
        if (fg_material.pbrData.metallicRoughnessTexture.has_value()) {
            material.texture_metallic_roughness_index = fg_material.pbrData.metallicRoughnessTexture->textureIndex;
        }
        material.rid = gApp->renderer->CreateMaterial(gpu_material);
    }
    return {};
}

Result<> glTF::LoadMeshes(const fastgltf::Asset& p_asset) {
    meshes.resize(p_asset.meshes.size());
    for (uint i = 0; i < p_asset.meshes.size(); ++i) {
        const fastgltf::Mesh& fg_mesh = p_asset.meshes[i];
        glTF::Mesh& mesh = meshes[i];
        mesh.name = fg_mesh.name;
        for (uint i = 0; i < fg_mesh.primitives.size(); ++i) {
            const fastgltf::Primitive& fg_primitive = fg_mesh.primitives[i];
            glTF::Primitive primitive{};
            if (fg_primitive.materialIndex.has_value()) {
                primitive.material_index = fg_primitive.materialIndex.value();
            }
            IterateIndices(p_asset, fg_primitive, primitive);
            IteratePositions(p_asset, fg_primitive, primitive);
            IterateNormals(p_asset, fg_primitive, primitive);
            IterateTangents(p_asset, fg_primitive, primitive);
            IterateUVs(p_asset, fg_primitive, primitive);

            primitive.rid = gApp->renderer->CreateMesh(primitive.vertices, primitive.indices);
            mesh.primitives.push_back(primitive);
        }
    }
    return {};
}

Result<glTF>
glTF::FromFile(const std::string& p_path) {
    glTF gltf{};

    std::filesystem::path path(p_path);
    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None) {
        return Error(std::format("Could not load glTF file. fastgltf error: {}", fastgltf::getErrorMessage(data.error())));
    }

    auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
    if (asset.error() != fastgltf::Error::None) {
        return Error(std::format("Could not parse glTF file. fastgltf error: {}", fastgltf::getErrorMessage(asset.error())));
    }

    CHECK_RET(gltf.LoadNodes(asset.get())
                  .and_then([&gltf, &asset]() {
                      return gltf.LoadTextures(asset.get());
                  })
                  .and_then([&gltf, &asset]() {
                      return gltf.LoadMaterials(asset.get());
                  })
                  .and_then([&gltf, &asset]() {
                      return gltf.LoadMeshes(asset.get());
                  }));
    return gltf;
}

Result<Ref<Gauge::Node>> glTF::CreateNode() {
    std::vector<Ref<Gauge::Node>> instanced_nodes;
    instanced_nodes.resize(nodes.size());
    for (uint i = 0; i < nodes.size(); ++i) {
        instanced_nodes[i] = std::make_shared<Gauge::Node>();
        instanced_nodes[i]->name = nodes[i].name;
        instanced_nodes[i]->local_transform = nodes[i].transform;
        if (nodes[i].mesh.has_value()) {
            const glTF::Mesh& mesh = meshes[nodes[i].mesh.value()];
            Ref<MeshInstance> mesh_component = std::make_shared<MeshInstance>();
            for (const auto& primitive : mesh.primitives) {
                mesh_component->surfaces.emplace_back(MeshInstance::Surface{
                    .primitive = primitive.rid,
                    .material = materials[primitive.material_index.value_or(0)].rid,
                });
            }
            instanced_nodes[i]->components.push_back(mesh_component);
            mesh_component->node = instanced_nodes[i];
        }
    }

    std::vector<bool> has_parent;
    has_parent.resize(nodes.size());
    for (uint i = 0; i < nodes.size(); ++i) {
        for (uint child_index : nodes[i].children) {
            instanced_nodes[i]->AddChild(instanced_nodes[child_index]);
            has_parent[child_index] = true;
        }
    }

    Ref<Gauge::Node> root_node = std::make_shared<Gauge::Node>();
    root_node->name = name;
    for (uint i = 0; i < nodes.size(); ++i) {
        if (!has_parent[i]) {
            root_node->AddChild(instanced_nodes[i]);
        }
    }

    return root_node;
}