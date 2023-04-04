#pragma once
#include "Metrics.h"
#include "Interfaces.h"
namespace RLL
{
	struct Color : public Math3D::Quaternion
	{
		void SetOpacity(float opacity)
		{
			w = 1.f - opacity;
		}
		void SetColor(float r, float g, float b, float a = 0)
		{
			x = r;
			y = g;
			z = b;
			w = a;
		}
		Color()
		{};
		Color(float r, float g, float b, float a)
		{
			x = r;
			y = g;
			z = b;
			w = a;
		};
	};
	struct ColorGradient
	{
		Color colors[8];
		float positions[8] = { 0.f,2.f };
		ColorGradient()
		{
			for (int i = 1; i < 8; i++)
			{
				positions[i] = 2;
			}
		}
	};
	struct RectangleI
	{
		int left;
		int top;
		int right;
		int bottom;
	};

	struct Rectangle : public Math3D::Quaternion
	{

	};

	class ITexture : public IBase
	{
		virtual void Dispose() = 0;

	};
	enum class JOINT_TYPE
	{
		FLAT = 0,
		MITER,
		BEVEL,
		ROUND
	};
	enum class CAP_TYPE
	{
		NONE = 0,
		SQUARE,
		TRIANGLE,
		ROUND
	};
	enum class ARC_TYPE
	{
		NEAREST,
		POSITIVE,
		NEGATIVE
	};
	struct StrokeStyle
	{
		JOINT_TYPE joint = JOINT_TYPE::FLAT;
		CAP_TYPE cap = CAP_TYPE::NONE;
	};
	class IBrush : public IBase
	{
	public:
		virtual void Dispose() = 0;

	};
	class IGeometry;
	class ISVG;
	class IGeometryBuilder : public IBase
	{
	public:
		virtual void Begin(Math3D::Vector2 p) = 0;
		virtual void End(bool close) = 0;
		virtual void LineTo(Math3D::Vector2 to) = 0;
		virtual void QuadraticTo(Math3D::Vector2 control, Math3D::Vector2 to) = 0;
		virtual void CubicTo(Math3D::Vector2 control0, Math3D::Vector2 control1, Math3D::Vector2 to) = 0;
		virtual void Ellipse(Math3D::Vector2 center, Math3D::Vector2 radius, bool inv = false) = 0;
		virtual void ArcTo(Math3D::Vector2 center, Math3D::Vector2 radius, float begin_rad, float end_rad, ARC_TYPE t = ARC_TYPE::NEAREST) = 0;
		virtual void Rectangle(Math3D::Vector2 lt, Math3D::Vector2 rb, bool inv = false) = 0;
		virtual void Triangle(Math3D::Vector2 p0, Math3D::Vector2 p1, Math3D::Vector2 p2, bool inv = false) = 0;
		virtual void RoundRectangle(Math3D::Vector2 lt, Math3D::Vector2 rb, Math3D::Vector2 radius, bool inv = false) = 0;
		virtual IGeometry* Fill() = 0;
		virtual IGeometry* Stroke(float stroke = 1, StrokeStyle* type = nullptr) = 0;
		virtual void Reset() = 0;
		virtual void Dispose() = 0;

	};
	class ISVGBuilder : public IBase
	{
	public:
		virtual void Push(IGeometry* geom, IBrush* brush = nullptr, Math3D::Matrix4x4* transform = nullptr) = 0;
		virtual void Push(ISVG* svg, Math3D::Matrix4x4* transform = nullptr) = 0;
		virtual void Reset() = 0;
		virtual ISVG* Commit() = 0;
		virtual void Dispose() = 0;
	};
	class IGeometry : public IBase
	{
		virtual void Dispose() = 0;

	};
	class ISVG : public IBase
	{
		virtual void Dispose() = 0;
	};

	class IPaintContext;
	class IPaintDevice
	{
	public:
		virtual IBrush* CreateSolidColorBrush(Color c) = 0;
		virtual IBrush* CreateDirectionalBrush(Math3D::Vector2 direction, ColorGradient* grad) = 0;
		virtual IBrush* CreateRadialBrush(Math3D::Vector2 center, float radius, ColorGradient* grad) = 0;
		virtual IBrush* CreateSweepBrush(Math3D::Vector2 center, float degree, ColorGradient* grad) = 0;
		virtual IBrush* CreateTexturedBrush(ITexture* tex, void* sampleMode) = 0;
		virtual ITexture* CreateTexture() = 0;
		virtual ITexture* CreateTextureFromFile() = 0;
		virtual ITexture* CreateTextureFromStream() = 0;
		virtual ITexture* CreateTextureFromMemory() = 0;

		virtual IGeometryBuilder* CreateGeometryBuilder() = 0;
		virtual ISVGBuilder* CreateSVGBuilder() = 0;

		virtual IPaintContext* CreateContext() = 0;

		virtual void CopyTexture(ITexture* src, ITexture* dst) = 0;
		virtual void CopyTextureRegion(ITexture* src, ITexture* dst, RectangleI) = 0;

		//virtual void Flush() = 0;
	};
	class IPaintContext
	{
		virtual IPaintDevice* GetDevice() = 0;
		//virtual void SetRenderTarget(ITexture* rt) = 0;
		//virtual ITexture* GetRenderTarget() = 0;
		virtual void BeginDraw(Color clear = {}) = 0;
		//virtual void PushLayer(Rectangle clip) = 0;
		//virtual void PopLayer() = 0;
		virtual void Done() = 0;
		virtual void EndDraw() = 0;
		virtual void DrawSVG(ISVG* svg) = 0;
		//virtual void DrawGeometry(IGeometry* geom) = 0;

	};
}