#pragma once
#include "Metrics.h"
#include <algorithm>
#include <functional>

namespace util
{
	Math3D::Vector2 Lerp(float t, Math3D::Vector2 a, Math3D::Vector2 b)
	{
		return a + (b - a) * t;
	}
	Math3D::Vector2 QuadraticBezier(float t, Math3D::Vector2 b, Math3D::Vector2 c, Math3D::Vector2 e)
	{
		return Lerp(t, Lerp(t, b, c), Lerp(t, c, e));
		// = b+(c-b)*t + c+(e-c)*t
	}
	Math3D::Vector2 QuadraticBezierTangent(float t, Math3D::Vector2 b, Math3D::Vector2 c, Math3D::Vector2 e)
	{
		return  (Lerp(t, c, e) - Lerp(t, b, c)) * 2;
		//= c + (e - c) * t - ( b + (c - b) * t) 
		//= c + e * t - c * t - b - c * t + b * t
		//= t*(e - 2c + b) + c - b
		//*2 = 2 * t(e - 2c + b) + 2c - 2b
		//' = 2(e - 2c + b)
	}
	Math3D::Vector2 dQBezierTangent(Math3D::Vector2 b, Math3D::Vector2 c, Math3D::Vector2 e)
	{
		return (e - c * 2 + b) * 2;
	}
	float CurvatureQBezier(float t, Math3D::Vector2 b, Math3D::Vector2 c, Math3D::Vector2 e)
	{
		auto dC = QuadraticBezierTangent(t, b, c, e);
		auto ddC = dQBezierTangent(b, c, e);
		float k = abs(dC.x * ddC.y - dC.y * ddC.x) / pow(dC.x * dC.x + dC.y * dC.y, 1.5f);
		return k;
	}
	Math3D::Vector2 CubicBezier(float t, Math3D::Vector2 b, Math3D::Vector2 c1, Math3D::Vector2 c2, Math3D::Vector2 e)
	{
		return Lerp(t, QuadraticBezier(t, b, c1, c2), QuadraticBezier(t, c1, c2, e));
	}
	void TrianglePPTT(
		Math3D::Vector2 pb, Math3D::Vector2 pe,
		Math3D::Vector2 tb, Math3D::Vector2 te,
		Math3D::Vector2& c)
	{
#define csconv(s) sqrtf(1 - (s)*(s))
		auto be = pe - pb;
		auto cb = Math3D::Vector2::Dot(be, tb) / be.Magnitude() / tb.Magnitude();
		auto ce = Math3D::Vector2::Dot(be, te) / be.Magnitude() / te.Magnitude();
		auto b = acosf(cb);
		auto e = acosf(ce);
		auto sc = sinf(b + e);
		auto bc_l = sinf(e) * be.Magnitude() / sc;
		c = pb + tb.Normalized() * bc_l;
	}

	float MaximalResolve(std::function<float(float)> eq, float mi, float ma)
	{
		float ep = 0.0001f;
		float l = mi; float r = ma;
		while (abs(l - r) > ep)
		{
			float m = 0.5f * (l + r);
			float vl = eq(l);
			float vr = eq(r);
			if (vl > vr)
				r = m;
			else
				l = m;
			auto dbg = vl + vr;
		}
		return 0.5f * (l + r);
	}

	float dCurQB(float t, Math3D::Vector2 b, Math3D::Vector2 c, Math3D::Vector2 e)
	{
		auto dC = QuadraticBezierTangent(t, b, c, e);
		auto ddC = dQBezierTangent(b, c, e);
		float u = ((ddC.x * dC.x + ddC.y * dC.y) * -3.f);
		float v = powf(dC.x * dC.x + dC.y * dC.y, 2.5f);
		return u / v;
	}

	Math3D::Vector2 Ellipse(Math3D::Vector2 radius, float r)
	{
		return Math3D::Vector2(radius.x * cosf(r), radius.y * sinf(r));
	}

	float EllipseDistance(Math3D::Vector2 radius, float rad)
	{
		return sqrtf(radius.x * cosf(rad) * radius.x * cosf(rad) + radius.y * radius.y * sinf(rad) * sinf(rad));
	}

	Math3D::Vector2 EllipseTangent(Math3D::Vector2 radius, float rad)
	{
		return Math3D::Vector2(radius.x * -sinf(rad), radius.y * cosf(rad));
	}
	Math3D::Vector2 Normal(Math3D::Vector2 v)
	{
		return ((v)*Math3D::Matrix4x4::Rotation(0, 0, -3.1415926f / 2));
	}

	//return seg count
	int QBezierExpand(Math3D::Vector2 b, Math3D::Vector2 c, Math3D::Vector2 e, float s,
		Math3D::Vector2* o_b, Math3D::Vector2* o_c, Math3D::Vector2* o_e)
	{
		auto maxt = util::MaximalResolve([&](float t) {return util::CurvatureQBezier(t, b, c, e); }, 0, 1);
		auto maxc = util::CurvatureQBezier(maxt, b, c, e) * (e - b).Magnitude();

		if (maxc > 3.f)
		{
			// 2 seg
			auto rb1 = b;
			auto rb2 = util::QuadraticBezier(maxt, b, c, e);
			auto rb3 = e;

			auto tb1 = (c - b).Normalized();
			auto tb2 = util::QuadraticBezierTangent(maxt, b, c, e).Normalized();
			auto tb3 = (e - c).Normalized();

			auto nb1 = Normal(tb1).Normalized();
			auto nb2 = Normal(tb2).Normalized();
			auto nb3 = Normal(tb3).Normalized();

			auto pb1 = rb1 + nb1 * s;
			auto pb2 = rb2 + nb2 * s;
			auto pb3 = rb3 + nb3 * s;

			Math3D::Vector2 c1, c2;
			util::TrianglePPTT(pb1, pb2, tb1, tb2, c1);
			util::TrianglePPTT(pb2, pb3, tb2, tb3, c2);
			o_b[0] = pb1;
			o_c[0] = c1;
			o_e[0] = pb2;

			o_b[1] = pb2;
			o_c[1] = c2;
			o_e[1] = pb3;
			return 2;
		}
		else if (maxc > 0.0001f)
		{
			//1 seg 
			auto tb = c - b;
			auto te = e - c;
			auto nb = Normal(tb).Normalized();
			auto ne = Normal(te).Normalized();

			Math3D::Vector2 con;
			util::TrianglePPTT(b + nb * s, e + ne * s,
				tb, te,
				con
			);
			o_b[0] = b + nb * s;
			o_c[0] = con;
			o_e[0] = e + ne * s;
			return 1;
		}
		else
		{
			auto tb = c - b;
			auto te = e - c;
			auto nb = Normal(tb).Normalized();
			auto ne = Normal(te).Normalized();

			o_b[0] = b + nb * s;
			o_e[0] = e + ne * s;
			o_c[0] = o_b[0] + tb;
			return 1;
		}
		return 0;
	}
}

//