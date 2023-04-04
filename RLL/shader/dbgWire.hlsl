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



struct VertexIn
{
	float3 PosL : POSITION;
	float3 pnc : PNB; // path normal color

	float2 uv : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 uv : TEXCOORD;
	nointerpolation float3 pnc : PNB;
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
	  (s * (m[0][0] * n.x + m[1][0] * n.y)
	 - t * (m[0][0] * p.x + m[1][0] * p.y + m[3][0]));
	
	float v = vwh.y * 
	  (s * (m[0][1] * n.x + m[1][1] * n.y)
	 - t * (m[0][1] * p.x + m[1][1] * p.y + m[3][1]));
	float d = (s * s * s * t + s * s * sqrt(u * u + v * v)) / (u * u + v * v - s * s * t * t + 0.0001);
	// vin.PosL += float3(1.2f, 0, 0) * vin.path_norm.x;
	p += float3(n * d, 0);

	// s = m[0][3] * uv.x + m[1][3] * uv.y + m[3][3];
	// t = m[0][3] * n.x + m[1][3] * n.y;
	// u = vwh.x * (s * (m[0][0] * n.x + m[1][0] * n.y) - t * (m[0][0] * uv.x + m[1][0] * uv.y + m[3][0]));
	// v = vwh.y * (s * (m[0][1] * n.x + m[1][1] * n.y) - t * (m[0][1] * uv.x + m[1][1] * uv.y + m[3][1]));
	// d = (s * s * s * t + s * s * sqrt(u * u + v * v)) / (u * u + v * v - s * s * t * t + 0.0001);
	//uv += n * d;
	vout.uv = uv;
	vout.PosH = mul(float4(p, 1.0f), m); //+ float4(d * n.x / 400.0, d * n.y / 300., 0, 0);
	vout.pnc = vin.pnc;
	return vout;
}
float4 PS(VertexOut pin) :SV_Target
{
	return float4(0.5,0.5,0.5,1);
}