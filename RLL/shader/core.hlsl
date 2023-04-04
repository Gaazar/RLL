cbuffer cbObject : register(b0)
{
	float4x4 objToWorld;
	int sampleType;
	float lerpTime;
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
	uint type;	// color mode: 0:foreground 1:directional 2:radial 3:sweep 4:tex
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
SamplerState g_sampler : register(s0);

Texture2D g_texture[1024] : register(t0); // albedo normal ORM
StructuredBuffer<Path> glyphs : register(t0, space1);
StructuredBuffer<Curve> curves : register(t1, space1);
StructuredBuffer<Brush> brushes : register(t1, space2);

struct VertexIn
{
	float3 PosL : POSITION;
	float3 pnc : COLOR; // path normal color
	// float3 norms : NORMAL;//vNormal uvNormal uvNorLen
	float2 uv : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 uv : TEXCOORD;
	nointerpolation float3 pnc : COLOR;
};
VertexOut VS(VertexIn vin, uint id
			 : SV_InstanceID)
{
	VertexOut vout;
	float2 n = float2(cos(vin.pnc.y), sin(vin.pnc.y));
	float3 p = vin.PosL;
	float2 uv = vin.uv;
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

	// s = m[0][3] * uv.x + m[1][3] * uv.y + m[3][3];
	// t = m[0][3] * n.x + m[1][3] * n.y;
	// u = vwh.x * (s * (m[0][0] * n.x + m[1][0] * n.y) - t * (m[0][0] * uv.x + m[1][0] * uv.y + m[3][0]));
	// v = vwh.y * (s * (m[0][1] * n.x + m[1][1] * n.y) - t * (m[0][1] * uv.x + m[1][1] * uv.y + m[3][1]));
	// d = (s * s * s * t + s * s * sqrt(u * u + v * v)) / (u * u + v * v - s * s * t * t + 0.0001);
	uv += n * d;
	vout.uv = uv;
	vout.PosH = mul(float4(p, 1.0f), m); //+ float4(d * n.x / 400.0, d * n.y / 300., 0, 0);
	vout.pnc = vin.pnc;
	return vout;
}

float4 sampleColor(Brush c, float p)
{
	float4
		fin = lerp(c.stops[0], c.stops[1], clamp((p - c.pos[0]) / (c.pos[1] - c.pos[0]), 0, 1));
	fin = lerp(fin, c.stops[2], clamp((p - c.pos[1]) / (c.pos[2] - c.pos[1]), 0, 1));
	fin = lerp(fin, c.stops[3], clamp((p - c.pos[2]) / (c.pos[3] - c.pos[2]), 0, 1));

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
float4 PS(VertexOut pin) : SV_Target
{
	float2 uv = pin.uv;
	float3 uvw = float3(pin.uv.x, pin.uv.y, 1);

	float cov = 0;
	float2 pixelsPerEm = float2(1.0 / fwidth(pin.uv.x),
								1.0 / fwidth(pin.uv.y));
	int glyph_index = (int)pin.pnc.x;
	int curve_index = glyphs[glyph_index].hi;
	int curve_length = glyphs[glyph_index].hl;
	for (int i = 0; i < curve_length; i++)
	{

		float2 cb = curves[curve_index + i].begin - uv;
		float2 cc = curves[curve_index + i].control - uv;
		float2 ce = curves[curve_index + i].end - uv;
		if (max(max(cb.x, cc.x), ce.x) * pixelsPerEm.x < -0.5)
			break;

		uint code = (0x2E74U >> (((cb.y > 0.0) ? 2U : 0U) +
								 ((cc.y > 0.0) ? 4U : 0U) + ((ce.y > 0.0) ? 8U : 0U))) &
					3U;
		if (code != 0U)
		{
			float ax = cb.x - cc.x * 2.0 + ce.x;
			float ay = cb.y - cc.y * 2.0 + ce.y;
			float bx = cb.x - cc.x;
			float by = cb.y - cc.y;
			float ra = 1.0 / ay;

			float d = sqrt(max(by * by - ay * cb.y, 0.0));
			float t1 = (by - d) * ra;
			float t2 = (by + d) * ra;

			// If the polynomial is nearly linear, then solve -2b t + c = 0.

			if (abs(ay) < 0.0001)
				t1 = t2 = cb.y * 0.5 / by;

			// Calculate the x coordinates where C(t) = 0, and transform
			// them so that the current pixel corresponds to the range
			// [0,1]. Clamp the results and use them for root contributions.

			float x1 = (ax * t1 - bx * 2.0) * t1 + cb.x;
			float x2 = (ax * t2 - bx * 2.0) * t2 + cb.x;
			x1 = clamp(x1 * pixelsPerEm.x + 0.5, 0.0, 1.0);
			x2 = clamp(x2 * pixelsPerEm.x + 0.5, 0.0, 1.0);

			if ((code & 1U) != 0U)
				cov += x1;
			if (code > 1U)
				cov -= x2;
		}
	}
	curve_index = glyphs[glyph_index].vi;
	curve_length = glyphs[glyph_index].vl;

	for (i = 0; i < curve_length; i++)
	{
		float2 cb = curves[curve_index + i].begin - uv;
		float2 cc = curves[curve_index + i].control - uv;
		float2 ce = curves[curve_index + i].end - uv;
		if (max(max(cb.y, cc.y), ce.y) * pixelsPerEm.y < -0.5)
			break;
		uint code = (0x2E74U >> (((cb.x > 0.0) ? 2U : 0U) +
								 ((cc.x > 0.0) ? 4U : 0U) + ((ce.x > 0.0) ? 8U : 0U))) &
					3U;
		if (code != 0U)
		{
			float ax = cb.y - cc.y * 2.0 + ce.y;
			float ay = cb.x - cc.x * 2.0 + ce.x;
			float bx = cb.y - cc.y;
			float by = cb.x - cc.x;
			float ra = 1.0 / ay;

			float d = sqrt(max(by * by - ay * cb.x, 0.0));
			float t1 = (by - d) * ra;
			float t2 = (by + d) * ra;

			if (abs(ay) < 0.0001)
				t1 = t2 = cb.x * 0.5 / by;

			float x1 = (ax * t1 - bx * 2.0) * t1 + cb.y;
			float x2 = (ax * t2 - bx * 2.0) * t2 + cb.y;
			x1 = clamp(x1 * pixelsPerEm.y + 0.5, 0.0, 1.0);
			x2 = clamp(x2 * pixelsPerEm.y + 0.5, 0.0, 1.0);

			if ((code & 1U) != 0U)
				cov -= x1;
			if (code > 1U)
				cov += x2;
		}
	}
	float gscl = sqrt(clamp(abs(cov) * 0.5, 0.0, 1.0));
	// float gscl = pow(clamp(abs(cov), 0.0, 1.0), 0.4545); //gamma
	// float gscl = clamp(abs(cov), 0, 1);
	float4 color = float4(1, 0, 1, 1);
	Brush cg = brushes[pin.pnc.z];
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
	// color = float4(1,0,0,0);
	//  return float4(gradientSwe(uv,float2(0.5,0.50),time * 0.2).xyz * gscl,1);
	// return float4(frac(uv), 0, gscl * 0.5 + 0.5);
	return float4(color.xyz, (1 - color.w) * gscl);
}

/*
uv0: brush uv
uv1: path coordinate

glyph data:*
	glyph index*    ushort
	glyph paths*    buffer

fill
*/