#pragma once

#include <gauge/common.hpp>
#include <gauge/core/handle.hpp>
#include <gauge/math/common.hpp>

namespace Gauge {

struct GPUImage;

struct Vertex {
    Vec3 position;
    float uv_x;
    Vec3 normal;
    float uv_y;
    Vec4 tangent;
};

struct PositionVertex {
    Vec3 position;
    float padding;

    PositionVertex() {}
    PositionVertex(Vec3 p_position) : position(p_position) {}
    PositionVertex(float x, float y, float z) : position(x, y, z) {}
};

struct CPUMesh {
    std::vector<Vertex> vertices;
    std::vector<uint> indices;
};

struct GPUMaterial {
    uint type;
    uint id;
};

struct GPU_PBRMaterial {
    Vec4 albedo = Vec4(1.0f);
    float metallic = 0.0f;
    float roughness = 0.0f;
    uint texture_albedo = 0;
    uint texture_normal = 2;
};

struct GPU_BasicMaterial {
    Vec4 color = Vec4(1.0f);
};

}  // namespace Gauge