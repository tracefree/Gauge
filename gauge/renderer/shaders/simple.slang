#include "input_structures.slang"
#include "limits.h"

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

struct Push {
    float4x4 model_matrix;
    Vertex* vertices;
    Sampler sampler;
    uint material_id;
    uint camera_id;
    uint scene_id;
}

ConstantBuffer<SamplerState[]> samplers;
ConstantBuffer<Texture2D[]> textures;
StructuredBuffer<Material> materials;

[[vk::binding(0, 1)]]
ConstantBuffer<Globals> globals;

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;

struct VertexOutput {
    float4 position_cs : SV_Position;
    float3 position_ws;
    float3 position_vs;
    float3x3 tbn_ws : TBN_WS;
    float2 uv : VertexUV;
};

struct FragmentOutput {
    float4 Albedo : COLOR0;
};

[shader("vertex")]
VertexOutput VertexMain(uint vertexID: SV_VertexID) {
    let vertex = push_constants.vertices[vertexID];
    let model_matrix = push_constants.model_matrix;
    var tbn_ws = float3x3(
        normalize(mul(model_matrix, float4(vertex.tangent.xyz, 0.0)).xyz),
        normalize(mul(model_matrix, float4(cross(vertex.normal, vertex.tangent.xyz) * vertex.tangent.w, 0.0)).xyz),
        normalize(mul(model_matrix, float4(vertex.normal, 0.0)).xyz)
    );

    let world_position = mul(push_constants.model_matrix, float4(vertex.position, 1.0));
    
    return VertexOutput(
        mul(globals.cameras[push_constants.camera_id].view_projection, world_position),
        world_position.xyz,
        mul(globals.cameras[push_constants.camera_id].view, world_position).xyz,
        tbn_ws,
    //    vertex.color,
        float2(vertex.uv_x, vertex.uv_y)
    );
}

[shader("fragment")]
FragmentOutput FragmentMain(
    float4 position_cs: SV_Position,
    float3 position_ws,
    float3 position_vs,
    float3x3 tbn_ws: TBN_WS,
    float2 uv: VertexUV,
)
{
    let material = materials[push_constants.material_id];

    let albedo_texture = textures[material.texture_albedo].Sample(samplers[(int)push_constants.sampler], uv);

    var albedo = material.albedo * albedo_texture;
    let normal_texture = textures[material.texture_normal].Sample(samplers[(int)push_constants.sampler], uv).xyz;
    let normal_ts = normal_texture.xyz * 2.0 - 1.0;
    let normal_ws = mul(normal_ts, tbn_ws);

    Camera camera = globals.cameras[push_constants.camera_id];
    float3 normal_vs = normalize(mul(camera.view, float4(normal_ws, 0.0)).xyz);
    float3 light_position = float3(1.5 * cos(globals.time), 1.5, 1.5 * sin(globals.time));
    float3 light_vector = light_position - position_ws;
    float light_distance = length(light_vector);
    float3 light_direction_ws = light_vector / light_distance;
    
    float3 light_direction_vs = mul(camera.view, float4(light_direction_ws, 0.0)).xyz;

    float ambient = 0.05;
    float point = max(dot(normal_vs, light_direction_vs), 0.0) * (1 / light_distance);
    
    float3 color = albedo.rgb * (ambient + point);

    return FragmentOutput(float4(color, 1.0));
}