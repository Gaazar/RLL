cbuffer cbObject : register(b0)
{
    float4x4 objToWorld;
    int sampleType;
    float timeBegin;
    float timePeriod;
    int timeLoop;
}

// cbuffer cbPass :register(b1)
//{
//
//
// };

cbuffer cbFrame : register(b1)
{
    float4x4 gWorldViewProj;
    float4 dColor;
    float time;
    int2 vwh;
    float reservedCBF;
};

struct Brush
{
    float2 center; // in directional is direction
    float2 focus;
    float radius;
    uint type; // color mode: 0:foreground 1:directional 2:radial 3:sweep 4:tex
    uint texid; // texture index
    float pad;
    float pos[8];
    float4 stops[8];
};
struct Path
{
    uint hi;
    uint vi;
    uint hl;
    uint vl;
};
struct Curve
{
    float2 begin;
    float2 control;
    float2 end;
};

struct VertexIn
{
    float2 PosL : POSITION;
	// float3 norms : NORMAL;//vNormal uvNormal uvNorLen
    float2 uv : TEXCOORD;
    float4 tf : TENSOR;
    float3 pnc : PNB0; // path normal color
    float3 pnc1 : PNB1;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation float3 pnc : PNB0;
    nointerpolation float3 pnc1 : PNB1;
};

Texture2D g_texture[1024] : register(t0); // albedo normal ORM
StructuredBuffer<Path> glyphs : register(t0, space1);
StructuredBuffer<Curve> curves : register(t1, space1);
StructuredBuffer<Brush> brushes : register(t1, space2);

float4 sampleColor(Brush c, float p)
{
	//float p = fwidth(pos);
    float4
		fin = lerp(c.stops[0], c.stops[1], clamp((p - c.pos[0]) / (c.pos[1] - c.pos[0]), 0, 1));
    fin = lerp(fin, c.stops[2], clamp((p - c.pos[1]) / (c.pos[2] - c.pos[1]), 0, 1));
    fin = lerp(fin, c.stops[3], clamp((p - c.pos[2]) / (c.pos[3] - c.pos[2]), 0, 1));

    return fin;
}
float4 sampleColorAA(Brush c, float p)
{
    float dp = fwidth(p);
    float4
		fin = 0.333f * (sampleColor(c, frac(p - 0.333f * dp)) + sampleColor(c, frac(p)) + sampleColor(c, frac(p + 0.333f * dp)));

    return fin;
}
float4 gradientDir(Brush c, float2 uv, float2 direction)
{
    return sampleColor(c, frac(uv.x * direction.x + uv.y * direction.y));
}
float4 gradientRad(Brush clr, float2 uv, float2 center, float radius, float2 focus)
{
	/// APPLY FOCUS MODIFIER
	// project a point on the circle such that it passes through the focus and through the coord,
	// and then get the distance of the focus from that point.
	// that is the effective gradient length
    float gradLength = 1.0;
    float2 diff = focus - center;
    float2 rayDir = normalize(uv - focus);
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(rayDir, diff);
    float c = dot(diff, diff) - radius * radius;
    float disc = b * b - 4.0 * a * c;
    if (disc >= 0.0)
    {
        float t = (-b + sqrt(abs(disc))) / (2.0 * a);
        float2 projection = focus + rayDir * t;
        gradLength = distance(projection, focus);
    }
    else
    {
		// gradient is undefined for this coordinate
    }

	/// OUTPUT
    float grad = distance(uv, focus) / gradLength;
    return sampleColor(clr, frac(grad));
}
float4 gradientSwe(Brush c, float2 uv, float2 center, float axis)
{
    uv = uv - center;
    float angel = atan2(uv.x, uv.y);
    return sampleColor(c, frac((angel / 3.1415926 / 2) + axis - 0.25));
}
