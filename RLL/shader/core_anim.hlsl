#include "path.hlsli"
SamplerState g_sampler : register(s0);

float4 PS(VertexOut pin) : SV_Target
{
    float2 uv = pin.uv;
    float3 uvw = float3(pin.uv.x, pin.uv.y, 1);

    float cov = 0;
    float2 pixelsPerEm = float2(1.0 / fwidth(pin.uv.x),
								1.0 / fwidth(pin.uv.y));
    int glyph_index = (int) pin.pnc.x;
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