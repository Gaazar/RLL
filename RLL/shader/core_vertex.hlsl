#include "path.hlsli"

VertexOut VS(VertexIn vin, uint id : SV_InstanceID)
{
    VertexOut vout;
    float2 n = float2(cos(vin.pLn.z), sin(vin.pLn.z));
    float3 p = float3(vin.pLn.xy, 0);
    float4x4 m = mul(objToWorld, gWorldViewProj);

    float s = m[0][3] * p.x + m[1][3] * p.y + m[3][3];
    float t = m[0][3] * n.x + m[1][3] * n.y;
    float u = vwh.x *
			  (s * (m[0][0] * n.x + m[1][0] * n.y) - t * (m[0][0] * p.x + m[1][0] * p.y + m[3][0]));

    float v = vwh.y *
			  (s * (m[0][1] * n.x + m[1][1] * n.y) - t * (m[0][1] * p.x + m[1][1] * p.y + m[3][1]));
    float d = (s * s * s * t + s * s * sqrt(u * u + v * v)) / (u * u + v * v - s * s * t * t + 0.0001);
	// vin.PosL += float3(1.2f, 0, 0) * vin.path_norm.x;
    p += float3(n * d, 0);
	//uv += n * d;
    vout.uv = p;
    vout.pH = mul(float4(p, 1.0f), m);
    return vout;
}