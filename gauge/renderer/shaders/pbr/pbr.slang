#include "../input_structures.slang"

ConstantBuffer<SamplerState[]> samplers;
ConstantBuffer<Texture2D[]> textures;
StructuredBuffer<void*> materials;

[[vk::binding(0, 1)]]
ConstantBuffer<Globals> globals;

[[vk::binding(1, 1)]]
RWStructuredBuffer<Readback> readback;

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

struct VertexOutput {
    float4 position_cs : SV_Position;
    float3 position_ws;
    float3 position_vs;
    float3x3 tbn_ws;
    float2 uv;
};

[shader("vertex")]
VertexOutput VertexMain(uint vertexID: SV_VertexID) {
    let vertex = pcs.vertices[vertexID];
    let model_matrix = pcs.model_matrix;
    var tbn_ws = float3x3(
        normalize(mul(model_matrix, float4(vertex.tangent.xyz, 0.0)).xyz),
        normalize(mul(model_matrix, float4(cross(vertex.normal, vertex.tangent.xyz) * vertex.tangent.w, 0.0)).xyz),
        normalize(mul(model_matrix, float4(vertex.normal, 0.0)).xyz)
    );

    let world_position = mul(pcs.model_matrix, float4(vertex.position, 1.0));
    
    return VertexOutput(
        mul(globals.cameras[pcs.camera_id].view_projection, world_position),
        world_position.xyz,
        mul(globals.cameras[pcs.camera_id].view, world_position).xyz,
        tbn_ws,
        float2(vertex.uv_x, vertex.uv_y)
    );
}

[shader("fragment")]
float4 FragmentMain(
    float4 position_cs: SV_Position,
    float3 position_ws,
    float3 position_vs,
    float3x3 tbn_ws,
    float2 uv,
) : COLOR0
{
    let material = GetMaterial<PBRMaterial>(pcs.material_handle);

    let albedo_texture = textures[material.texture_albedo].Sample(samplers[Sampler::LINEAR], uv);
    var albedo = material.albedo * albedo_texture;

    let normal_texture = textures[material.texture_normal].Sample(samplers[Sampler::LINEAR], uv).xyz;
    let normal_ts = normal_texture.xyz * 2.0 - 1.0;
    let normal_ws = mul(normal_ts, tbn_ws);

    Camera camera = globals.cameras[pcs.camera_id];
    float3 normal_vs = normalize(mul(camera.view, float4(normal_ws, 0.0)).xyz);
    float3 light_position = float3(1.5 * cos(globals.time), 1.5, 1.5 * sin(globals.time));
    float3 light_vector = light_position - position_ws;
    float light_distance = length(light_vector);
    float3 light_direction_ws = light_vector / light_distance;
    
    float3 light_direction_vs = mul(camera.view, float4(light_direction_ws, 0.0)).xyz;

    float ambient = 0.05;
    float point = max(dot(normal_vs, light_direction_vs), 0.0) * (1 / light_distance);
    
    float3 color = albedo.rgb * (ambient + point);

    if (pcs.mouse_position.x == uint16_t(position_cs.x) && pcs.mouse_position.y == uint16_t(position_cs.y)) {
        uint index = readback[0].node_handle.load();
        index++;
        readback[index].node_handle = pcs.node_handle;
        readback[index].depth = position_cs.z;
        readback[0].node_handle++;
    }

    return float4(color, 1.0);
}