#include "LightingCommon.hlsli"

struct VSInput
{
    float3 pos  : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv   : TEXCOORD0;
};

struct VSOutput
{
    float4 pos      : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 tang     : TEXCOORD1;
    float3 norm     : TEXCOORD2;
    float2 uv       : TEXCOORD3;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    float4 worldPos = mul(model, float4(vertex.pos, 1.0f));
    result.pos = mul(vp, worldPos);
    result.worldPos = worldPos.xyz;
    result.tang = mul(normalModel, float4(vertex.tang, 0.0f)).xyz;
    result.norm = mul(normalModel, float4(vertex.norm, 0.0f)).xyz;
    result.uv = vertex.uv;

    return result;
}