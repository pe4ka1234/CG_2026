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
    Light lights[10];
    float4 ambientColor;
};

float3 CalculateColor(float3 color, float3 normal, float3 worldPos, float shininess, bool unusedFlag)
{
    float3 finalColor = ambientColor.xyz * color;
    float3 n = normalize(normal);

    [unroll]
    for (int i = 0; i < lightCount.x; ++i)
    {
        float3 lightDir = lights[i].pos.xyz - worldPos;
        float lightDistSq = max(dot(lightDir, lightDir), 1e-4f);
        float lightDist = sqrt(lightDistSq);
        lightDir /= lightDist;

        float atten = 1.0f / lightDistSq;

        float diffuse = max(dot(lightDir, n), 0.0f);
        finalColor += color * diffuse * atten * lights[i].color.xyz;

        float3 viewDir = normalize(cameraPos.xyz - worldPos);
        float3 reflectDir = reflect(-lightDir, n);
        float spec = shininess > 0.0f ? pow(max(dot(viewDir, reflectDir), 0.0f), shininess) : 0.0f;

        finalColor += color * spec * atten * lights[i].color.xyz;
    }

    return saturate(finalColor);
}