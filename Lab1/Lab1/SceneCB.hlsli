struct Light
{
    float4 pos;
    float4 color;
};

cbuffer SceneBuffer : register(b0)
{
    float4x4 vp;
    float4 cameraPos;
    int4 lightCount;   
    int4 postProcess;  
    Light lights[10];
    float4 ambientColor;
    float4 frustum[6];
};