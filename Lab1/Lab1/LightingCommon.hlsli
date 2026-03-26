struct Light
{
    float4 pos;
    float4 color;
};

cbuffer GeomBuffer : register(b0)
{
    float4x4 model;
    float4 size;
    float4 color;
    float4x4 normalModel;
    float4 materialParams; // x - shininess
};

cbuffer SceneBuffer : register(b1)
{
    float4x4 vp;
    float4 cameraPos;
    int4 lightCount;
    Light lights[10];
    float4 ambientColor;
};

float3 ComputePhongLighting(float3 worldPos, float3 normal, float3 albedo)
{
    float3 finalColor = ambientColor.xyz * albedo;

    float3 viewDir = normalize(cameraPos.xyz - worldPos);

    [unroll]
    for (int i = 0; i < lightCount.x; ++i)
    {
        float3 lightVector = lights[i].pos.xyz - worldPos;
        float lightDistSq = max(dot(lightVector, lightVector), 1e-4f);
        float lightDist = sqrt(lightDistSq);
        float3 lightDir = lightVector / lightDist;
        float atten = 1.0f / lightDistSq;

        float diffuse = max(dot(lightDir, normal), 0.0f);
        finalColor += albedo * diffuse * atten * lights[i].color.xyz;

        float spec = 0.0f;
        if (materialParams.x > 0.0f && diffuse > 0.0f)
        {
            float3 reflectDir = reflect(-lightDir, normal);
            spec = pow(max(dot(viewDir, reflectDir), 0.0f), materialParams.x);
        }

        finalColor += albedo * spec * atten * lights[i].color.xyz;
    }

    return finalColor;
}