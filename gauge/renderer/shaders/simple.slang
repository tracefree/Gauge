#include "input_structures.slang"

struct Push {
    float4x4 model_matrix;
    float4x4 view_projection;
    Vertex* vertices;
}

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
        mul(push_constants.view_projection, world_position),
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
   // let albedo_texture_sample = fragment_params.albedo_texture.Sample(uv);
   // let albedo = fragment_params.material_data.albedo_factors.rgb * color.rgb * albedo_texture_sample.rgb;
    let normal_ts = float3(0.0, 0.0, 1.0);
    let normal_ws = mul(normal_ts, tbn_ws);
    return FragmentOutput(float4(normal_ws, 1.0));
}