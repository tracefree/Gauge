#include "gltf.hpp"

#include <gauge/common.hpp>
#include <gauge/components/aabb_gizmo.hpp>
#include <gauge/components/mesh_instance.hpp>
#include <gauge/core/app.hpp>
#include <gauge/core/handle.hpp>
#include <gauge/core/resource_manager.hpp>
#include <gauge/math/common.hpp>
#include <gauge/renderer/common.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include "fastgltf/math.hpp"
#include "fastgltf/util.hpp"
#include "thirdparty/stb/stb_image.h"

#include <assert.h>
#include <cstdio>
#include <memory>
#include <print>
#include <variant>

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

    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset,
        accessor,
        [&](Vec3 position, size_t index) {
            primitive.vertices[index].position = position;
            primitive.aabb.Grow(position);
        });
}

static void IterateNormals(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    auto attribute = fg_primitive.findAttribute(Attribute::NORMAL);
    if (attribute == fg_primitive.attributes.end())
        return;
    const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];
    fastgltf::iterateAccessorWithIndex<glm::vec3>(
        asset,
        accessor,
        [&](Vec3 normal, size_t index) {
            primitive.vertices[index].normal = normal;
        });
}

static void IterateTangents(const fastgltf::Asset& asset, const fastgltf::Primitive& fg_primitive, glTF::Primitive& primitive) {
    auto attribute = fg_primitive.findAttribute(Attribute::TANGENT);
    if (attribute == fg_primitive.attributes.end()) {
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
    fastgltf::iterateAccessorWithIndex<glm::vec2>(
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

Result<> glTF::LoadTextures(const fastgltf::Asset& p_asset, const std::filesystem::path& p_path) {
    textures.resize(p_asset.textures.size());
    for (uint i = 0; i < p_asset.textures.size(); ++i) {
        const fastgltf::Texture& fg_texture = p_asset.textures[i];
        glTF::Texture& texture = textures[i];
        auto fg_image = p_asset.images[fg_texture.imageIndex.value()];
        std::string err;
        std::visit(fastgltf::visitor{
                       [&](fastgltf::sources::URI& file_name) {
                           auto file_path = StringID(p_path / file_name.uri.fspath());
                           texture.data = ResourceManager::Load<Gauge::Texture>(file_path);
                       },
                       [&](const fastgltf::sources::Array& array) {
                           std::println("Array!");
                           assert(false);
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
                                       texture.data->data = stbi_load_from_memory((stbi_uc*)array.bytes.data() + buffer_view.byteOffset, static_cast<int>(buffer_view.byteLength), &width, &height, &number_channels, 4);
                                       texture.data->width = (uint)width;
                                       texture.data->height = (uint)width;
                                       if (!texture.data->data) {
                                           err = "Could not load texture: Couldn't load data from memory";
                                           texture.data->data = nullptr;
                                       }
                                   },
                               },
                               buffer.data);
                       },
                       [&](auto& argument) {
                           err = "Could not load texture: data source not implemented";
                           assert(false);
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
            texture.data->use_srgb = true;
            if (texture.handle.index == 0) {
                texture.handle = gApp->renderer->CreateTexture(*texture.data);
            }
            gpu_material.texture_albedo = texture.handle.index;
        }
        if (fg_material.normalTexture.has_value()) {
            material.texture_normal_index = fg_material.normalTexture->textureIndex;
            glTF::Texture& texture = textures[material.texture_normal_index.value()];
            texture.data->use_srgb = false;
            if (texture.handle.index == 0) {
                texture.handle = gApp->renderer->CreateTexture(*texture.data);
            }
            gpu_material.texture_normal = texture.handle.index;
        }
        if (fg_material.pbrData.metallicRoughnessTexture.has_value()) {
            material.texture_metallic_roughness_index = fg_material.pbrData.metallicRoughnessTexture->textureIndex;
        }
        material.handle = gApp->renderer->CreateMaterial(gpu_material);
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

            primitive.handle = gApp->renderer->CreateMesh(primitive.vertices, primitive.indices);
            mesh.primitives.push_back(primitive);
            mesh.aabb.Grow(primitive.aabb);
        }
    }
    return {};
}

Result<glTF>
glTF::FromFile(const std::string& p_path) {
    glTF gltf{};

    std::filesystem::path path(p_path);
    gltf.name = path.stem();
    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (data.error() != fastgltf::Error::None) {
        return Error(std::format("Could not load glTF file. fastgltf error: {}", fastgltf::getErrorMessage(data.error())));
    }

    auto asset = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::LoadExternalBuffers);
    if (asset.error() != fastgltf::Error::None) {
        return Error(std::format("Could not parse glTF file. fastgltf error: {}", fastgltf::getErrorMessage(asset.error())));
    }

    CHECK_RET(gltf.LoadNodes(asset.get())
                  .and_then([&gltf, &asset, &path]() {
                      return gltf.LoadTextures(asset.get(), path.parent_path());
                  })
                  .and_then([&gltf, &asset]() {
                      return gltf.LoadMaterials(asset.get());
                  })
                  .and_then([&gltf, &asset]() {
                      return gltf.LoadMeshes(asset.get());
                  }));
    return gltf;
}

Result<Ref<Gauge::Node>> glTF::CreateNode() const {
    std::vector<Ref<Gauge::Node>> instanced_nodes;
    instanced_nodes.resize(nodes.size());
    for (uint i = 0; i < nodes.size(); ++i) {
        instanced_nodes[i] = Gauge::Node::Create(nodes[i].name);
        instanced_nodes[i]->local_transform = nodes[i].transform;
        AABB aabb;
        if (nodes[i].mesh.has_value()) {
            const glTF::Mesh& mesh = meshes[nodes[i].mesh.value()];
            Ref<MeshInstance> mesh_component = std::make_shared<MeshInstance>();
            for (const auto& primitive : mesh.primitives) {
                mesh_component->surfaces.emplace_back(MeshInstance::Surface{
                    .primitive = primitive.handle,
                    .material = materials[primitive.material_index.value_or(0)].handle,
                });
            }
            instanced_nodes[i]->AddComponent(mesh_component);
            instanced_nodes[i]->aabb = mesh.aabb;
        }
    }

    std::vector<bool> has_parent;
    std::vector<bool> has_children;
    has_parent.resize(nodes.size());
    has_children.resize(nodes.size());
    for (uint i = 0; i < nodes.size(); ++i) {
        for (uint child_index : nodes[i].children) {
            instanced_nodes[i]->AddChild(instanced_nodes[child_index]);
            has_parent[child_index] = true;
            has_children[i] = true;
        }
    }

    auto root_node = Gauge::Node::Create(name);
    root_node->name = name;
    for (uint i = 0; i < nodes.size(); ++i) {
        if (!has_parent[i]) {
            root_node->AddChild(instanced_nodes[i]);
        }
    }

    for (uint i = 0; i < nodes.size(); ++i) {
        if (has_children[i]) {
            continue;
        }
        Ref<Gauge::Node> node = instanced_nodes[i];
        while (node->parent.lock() != nullptr) {
            Ref<Gauge::Node> child = node;
            node = node->parent.lock();
            if (child->aabb.IsValid()) {
                node->aabb.Grow(child->local_transform * child->aabb);
            }
        }
    }

    for (auto node : instanced_nodes) {
        node->AddComponent<AABBGizmo>(node->aabb)->visible = false;
    }
    root_node->AddComponent<AABBGizmo>(root_node->aabb)->visible = false;

    return root_node;
}