#include "../input_structures.slang"

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
    uint16_t2 mouse_position;
    MaterialHandle material_handle;
    uint camera_id;
    uint node_handle;
}

[[vk::push_constant]]
ConstantBuffer<PushConstants> pcs;

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
    if (pcs.mouse_position.x == uint16_t(position_cs.x) && pcs.mouse_position.y == uint16_t(position_cs.y)) {
        uint index = readback[0].node_handle.load();
        index++;
        readback[index].node_handle = pcs.node_handle;
        readback[index].depth = position_cs.z;
        readback[0].node_handle++;
    }

    let material = GetMaterial<BasicMaterial>(pcs.material_handle);
      
    return material.color;
}