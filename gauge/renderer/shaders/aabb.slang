#include "input_structures.slang"
#include "limits.h"

static const float3 positions[8] = {
    float3(-1.0, -1.0, -1.0), float3(-1.0, -1.0, 1.0), float3(1.0, -1.0, 1.0), float3(1.0, -1.0, -1.0),
    float3(-1.0, 1.0, -1.0), float3(-1.0, 1.0, 1.0), float3(1.0, 1.0, 1.0), float3(1.0, 1.0, -1.0),
};

static const uint indices[24] = {
    0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7
};

struct Push {
    float3 position;
    uint camera_id;
    float3 extent;
    float _padding1;
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

[[vk::binding(0, 1)]]
ConstantBuffer<Globals> globals;

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;

[shader("vertex")]
float4 VertexMain(uint vertex_id: SV_VertexID) : SV_Position {
    let world_position = float4(push_constants.position + push_constants.extent * positions[indices[vertex_id]], 1.0);
    return mul(globals.cameras[push_constants.camera_id].view_projection, world_position);
}

[shader("fragment")]
float4 FragmentMain() : COLOR_0
{
    return float4(1.0, 0.5, 0.0, 1.0);
}
