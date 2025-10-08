#include "../input_structures.slang"
#include "../limits.h"

struct PushConstants {
    float4x4 model_matrix;
    float4* vertices;
    uint camera_id;
    uint padding;
    float4 color;
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
ConstantBuffer<PushConstants> pcs;

[shader("vertex")]
float4 VertexMain(uint vertex_id: SV_VertexID) : SV_Position {
    let world_position = mul(pcs.model_matrix, float4(pcs.vertices[vertex_id].xyz, 1.0));
    return mul(globals.cameras[pcs.camera_id].view_projection, world_position);
}

[shader("fragment")]
float4 FragmentMain() : COLOR_0
{
    return pcs.color;
}
