#include "input_structures.slang"
#include "limits.h"

struct Camera {
    float4x4 view;
    float4x4 view_projection;
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
    LINEAR,
    NEAREST,
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
        tbn_ws,
    //    vertex.color,
        float2(vertex.uv_x, vertex.uv_y)
    );
}

[shader("fragment")]
FragmentOutput FragmentMain(
    float4 position_cs: SV_Position,
    float3x3 tbn_ws: TBN_WS,
    float2 uv: VertexUV,
)
{
    let material = materials[push_constants.material_id];
    let albedo_texture = textures[material.texture_albedo].Sample(samplers[push_constants.sampler], uv);
    var albedo = material.albedo * albedo_texture;
    let normal_texture = textures[material.texture_normal].Sample(samplers[push_constants.sampler], uv).xyz;
    let normal_ts = normal_texture.xyz * 2.0 - 1.0;
    let normal_ws = mul(normal_ts, tbn_ws);

    if (push_constants.sampler == 1) {
        albedo.rgb = normal_ws * 0.5 + 0.5;
    }

    albedo.rgb = sin(globals.time) * albedo.rgb + (1.0 - sin(globals.time)) * (float3(1.0) - albedo.rgb);

    return FragmentOutput(albedo);
}