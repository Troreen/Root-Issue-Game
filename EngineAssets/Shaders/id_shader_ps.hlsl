#include "common.hlsli"
SamplerState SampleType;
Texture2D shaderTextures[2] : register(t1);


cbuffer IdConstantBuffer : register(b13)
{
    uint objectID;
    uint selectionID;
    uint p4status;
    uint unused;
};

uint4 main(ModelVertexToPixel input) : SV_TARGET
{
    float4 Diffuse = shaderTextures[0].Sample(SampleType, input.texCoord0);
    if (Diffuse.w <= alphaTestThreshold)
    {
        discard;
    }
    return uint4(objectID, selectionID, p4status, 0);
}
