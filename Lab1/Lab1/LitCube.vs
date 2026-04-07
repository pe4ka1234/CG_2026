#include "LightingCommon.hlsli"

#define MAX_INST 100

struct GeomBuffer
{
    float4x4 model;
    float4x4 norm;
    float4 shineSpeedTexIdNM; // x - shininess, y - rotation speed, z - texture id, w - normal map presence
    float4 posAngle;          // xyz - position, w - current angle
};

cbuffer GeomBufferInst : register(b1)
{
    GeomBuffer geomBuffer[MAX_INST];
};

cbuffer GeomBufferInstVis : register(b2)
{
    uint4 ids[MAX_INST];
};

struct VSInput
{
    float3 pos : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;
    uint instanceId : SV_InstanceID;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 worldPos : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;
    nointerpolation uint instanceId : INST_ID;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    uint idx = ids[vertex.instanceId].x;
    float4 worldPos = mul(geomBuffer[idx].model, float4(vertex.pos, 1.0f));

    result.pos = mul(vp, worldPos);
    result.worldPos = worldPos;
    result.uv = vertex.uv;
    result.tang = mul(geomBuffer[idx].norm, float4(vertex.tang, 0.0f)).xyz;
    result.norm = mul(geomBuffer[idx].norm, float4(vertex.norm, 0.0f)).xyz;
    result.instanceId = idx;

    return result;
}