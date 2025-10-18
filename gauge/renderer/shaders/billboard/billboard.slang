#include "../input_structures.slang"
#include "../node_select.slang"

struct PushConstants {
    float3 world_position;
    uint camera_id;
    float2 size;
    MaterialHandle material_handle;
    uint node_handle;
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
    write_hovered_node(position_cs.xyz);

    let material = GetMaterial<BillboardMaterial>(pcs.material_handle);
    let texture = textures[material.texture].Sample(samplers[Sampler::LINEAR], uv);
    return material.color * texture;
}
