static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

struct VertexOutput {
    float4 sv_position : SV_Position;
};

struct Push {
    float4 color;
}

[[vk::push_constant]]
ConstantBuffer<Push> push_constants;

[shader("vertex")]
VertexOutput VertexMain(uint vertex_id : SV_VertexID) {
    VertexOutput output;
    output.sv_position = float4(positions[vertex_id], 0.0, 1.0);
    return output;
}

[shader("fragment")]
float4 FragmentMain() : SV_Target
{
    return push_constants.color;
}