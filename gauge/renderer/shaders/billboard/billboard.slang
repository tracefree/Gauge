#include "../input_structures.slang"

struct PushConstants {
    float3 world_position;
    uint camera_id;
    float2 size;
    MaterialHandle material_handle;
    uint node_handle;
    uint16_t2 mouse_position;
    float2 _padding0;
};

ConstantBuffer<SamplerState[]> samplers;
ConstantBuffer<Texture2D[]> textures;
StructuredBuffer<void*> materials;

[[vk::binding(0, 1)]]
ConstantBuffer<Globals> globals;

[[vk::binding(1, 1)]]
RWStructuredBuffer<Readback> readback;

[[vk::push_constant]]
ConstantBuffer<PushConstants, ScalarDataLayout> pcs;

static float2 vertices[6] = {
    float2(0.5, 0.5), float2(0.5, -0.5), float2(-0.5, 0.5),
    float2(-0.5, -0.5), float2(0.5, -0.5), float2(-0.5, 0.5),
};

struct VertexOutput {
    float4 position_cs : SV_Position;
    float2 uv;
}

[shader("vertex")]
VertexOutput VertexMain(uint vertex_id: SV_VertexID) {
    let screen_position = mul(globals.cameras[pcs.camera_id].view_projection, float4(pcs.world_position, 1.0));
    let screen_offset = pcs.size * globals.cameras[pcs.camera_id].pixel_size * vertices[vertex_id] * screen_position.w;
    let uv = vertices[vertex_id] + 0.5;
    return VertexOutput(
        screen_position + float4(screen_offset, 0.0, 0.0),
        uv
    );
}

[shader("fragment")]
float4 FragmentMain(
    float4 position_cs : SV_Position,
    float2 uv
) : COLOR_0
{
    if (pcs.mouse_position.x == uint16_t(position_cs.x) && pcs.mouse_position.y == uint16_t(position_cs.y)) {
        uint index = readback[0].node_handle.load();
        index++;
        readback[index].node_handle = pcs.node_handle;
        readback[index].depth = position_cs.z;
        readback[0].node_handle++;
    }

    let material = GetMaterial<BillboardMaterial>(pcs.material_handle);
    let texture = textures[material.texture].Sample(samplers[Sampler::LINEAR], uv);
    return material.color * texture;
}
