#include "../input_structures.slang"
#include "../limits.h"

static const float GIZMO_SCALE = 0.2;

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

struct PushConstants {
    float4x4 model_matrix;
    Vertex* vertices;
    float2 color_rg;
    float color_b;
    uint camera_id;
    uint16_t2 mouse_position;
    uint node_handle;
}

struct Readback {
    uint node_handle;
    float depth;
}

ConstantBuffer<SamplerState[]> samplers;
ConstantBuffer<Texture2D[]> textures;
StructuredBuffer<Material> materials;

[[vk::binding(0, 1)]]
ConstantBuffer<Globals> globals;

[[vk::binding(1, 1)]]
RWStructuredBuffer<Readback> readback;

[[vk::push_constant]]
ConstantBuffer<PushConstants> pcs;

[shader("vertex")]
float4 VertexMain(uint vertexID: SV_VertexID) : SV_Position {
    let vertex = pcs.vertices[vertexID];
    let model_matrix = pcs.model_matrix;
    let scale = -GIZMO_SCALE * transpose(globals.cameras[0].view)[3].z;
    var world_position = mul(pcs.model_matrix, float4(vertex.position * scale, 1.0));
    
    return mul(globals.cameras[pcs.camera_id].view_projection, world_position);
}

[shader("fragment")]
float4 FragmentMain(
    float4 position_cs: SV_Position,
) : COLOR0
{
    if (pcs.mouse_position.x == uint16_t(position_cs.x) && pcs.mouse_position.y == uint16_t(position_cs.y)) {
        readback[readback[0].node_handle + 1].node_handle = pcs.node_handle;
        readback[readback[0].node_handle + 1].depth = position_cs.z;
        readback[0].node_handle = readback[0].node_handle + 1;
    }
    
    return float4(pcs.color_rg, pcs.color_b,  1.0);
}