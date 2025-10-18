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
    float2 pixel_size;
    float2 _padding0;
}

struct PointLight {
    float3 position;
    float range;
    float3 color;
    float intensity;
}

struct Scene {
    float3 ambient_light_color;
    float ambient_light_intensity;
    uint active_point_lights;
    float _padding1;
    float _padding2;
    float _padding3;
    PointLight point_lights[MAX_POINT_LIGHTS];
}

struct Globals {
    float time;
    uint16_t2 mouse_position;
    float _padding2;
    float _padding3;
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

struct BillboardMaterial {
    float4 color;
    uint texture;
}

T GetMaterial<T>(MaterialHandle material_handle) {
    T* material_list = reinterpret<T*>(materials[material_handle.type]);
    return material_list[material_handle.id];
}