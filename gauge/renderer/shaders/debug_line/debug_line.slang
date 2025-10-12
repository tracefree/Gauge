#include "../input_structures.slang"

struct PushConstants {
    float4x4 model_matrix;
    float4* vertices;
    uint camera_id;
    uint padding;
    float4 color;
}

StructuredBuffer<void*> materials;

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
