#pragma once

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
    Vec4 albedo;
    uint texture_albedo;
    uint texture_normal;
};

}  // namespace Gauge