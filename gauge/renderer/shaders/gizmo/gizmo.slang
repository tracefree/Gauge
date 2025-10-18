#include "../input_structures.slang"
#include "../node_select.slang"

ConstantBuffer<SamplerState[]> samplers;
ConstantBuffer<Texture2D[]> textures;
StructuredBuffer<void*> materials;

[[vk::binding(0, 1)]]
ConstantBuffer<Globals> globals;

[[vk::binding(1, 1)]]
RWStructuredBuffer<Readback> readback;

static const float GIZMO_SCALE = 0.2;

struct PushConstants {
    float4x4 model_matrix;
    Vertex* vertices;
    MaterialHandle material_handle;
    uint camera_id;
    uint node_handle;
}

[[vk::push_constant]]
ConstantBuffer<PushConstants, ScalarDataLayout> pcs;

[shader("vertex")]
float4 VertexMain(uint vertex_id: SV_VertexID) : SV_Position {
    let vertex = pcs.vertices[vertex_id];
    let model_matrix = pcs.model_matrix;
    let view_position = mul(globals.cameras[0].view, model_matrix);
    let scale = -GIZMO_SCALE * transpose(view_position)[3].z;
    var world_position = mul(pcs.model_matrix, float4(vertex.position * scale, 1.0));
    
    return mul(globals.cameras[pcs.camera_id].view_projection, world_position);
}

[shader("fragment")]
float4 FragmentMain(
    float4 position_cs: SV_Position,
) : COLOR0
{
    write_hovered_node(position_cs.xyz);

    let material = GetMaterial<BasicMaterial>(pcs.material_handle);
    return material.color;
}