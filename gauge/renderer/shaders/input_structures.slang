#include "limits.h"

struct Vertex {
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
  //  float4 color;
    float4 tangent;
}

struct Camera {
    float4x4 view;
    float4x4 view_projection;
    float4x4 inverse_projection;
}

struct Scene {
    float3 light_position;
    float light_attenuation;
}

struct Globals {
    float time;
    Camera cameras[MAX_CAMERAS];
    Scene scenes[MAX_SCENES];
}

enum Sampler {
    LINEAR = 0,
    NEAREST = 1,
}

struct Readback {
    Atomic<uint> node_handle;
    float depth;
}

struct MaterialHandle {
    uint type;
    uint id;
}

struct PBRMaterial {
  float4 albedo;
  float metallic;
  float roughness;
  uint texture_albedo;
  uint texture_normal;
}

struct BasicMaterial {
  float4 color;
}

T GetMaterial<T>(MaterialHandle material_handle) {
    T* material_list = reinterpret<T*>(materials[material_handle.type]);
    return material_list[material_handle.id];
}