#include "path.hlsli"
SamplerState g_sampler : register(s0);

float4 sampleBrush(Brush cg, float2 uv)
{
    float4 color = float4(1, 0, 1, 1);
    switch (cg.type & 0xff)
    {
        case 0:
            color = dColor;
            break;
        case 1:
            color = gradientDir(cg, uv, cg.center);
            break;
        case 2:
            color = gradientRad(cg, uv, cg.center, cg.radius, cg.focus);
            break;
        case 3:
            color = gradientSwe(cg, uv, cg.center, cg.radius);
            break;
        case 4:
            color = g_texture[cg.texid].Sample(g_sampler, uv);
            break;
    }
    return color;
}