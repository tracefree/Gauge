#pragma once

#include <gauge/common.hpp>
#include <gauge/math/common.hpp>

namespace Gauge {

struct Vertex {
    Vec3 position;
    float uv_x;
    Vec3 normal;
    float uv_y;
    Vec4 tangent;
};

struct CPUMesh {
    std::vector<Vertex> vertices;
    std::vector<uint> indices;
};

struct GPUMaterial {
    Vec4 albedo = Vec4(1.0f);
    float metallic = 0.0f;
    float roughness = 0.0f;
    RID texture_albedo = 0;
    RID texture_normal = 2;
};

}  // namespace Gauge