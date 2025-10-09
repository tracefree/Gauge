struct Vertex {
    float3 position;
    float uv_x;
    float3 normal;
    float uv_y;
  //  float4 color;
    float4 tangent;
}

struct PBRMaterial {
  float4 albedo;
  float metallic;
  float roughness;
  uint texture_albedo;
  uint texture_normal;
}