#pragma once
#include "math3d/Math3Df.h"
#ifdef _DEBUG
#define NOIMPL std::cout <<"[NOIMPL]\t"<< __FILE__ << "\tline:" << __LINE__ << "\t" << __FUNCTION__ << " not implemented yet." << std::endl;
#else
#define NOIMPL
#endif
namespace RLL
{
	enum SIZE_MODE {
		SIZE_MODE_CONTENT,	//size is calculated by inside content
		SIZE_MODE_FIXED,	//by given metrics
		SIZE_MODE_DYNAMIC	//by parent given
	};
	enum AXIS_DIR {
		AXIS_DIR_NORMAL,	//left to right or top to bottom
		AXIS_DIR_REVERSE,
		AXIS_DIR_S2E,	//start to end, text direction
		AXIS_DIR_E2S,	//end to start
	};
	enum AXIS_MAJOR
	{
		AXIS_MAJOR_ROW,
		AXIS_MAJOR_COLUMN
	};
	enum AXIS_SPACE
	{
		AXIS_SPACE_START,
		AXIS_SPACE_END,
		AXIS_SPACE_CENTER,
		AXIS_SPACE_BETWEEN,
		AXIS_SPACE_AROUND
	};
	enum LINE_ALIGN
	{
		LINE_ALIGN_START,
		LINE_ALIGN_END,
		LINE_ALIGN_BASELINE,
		LINE_ALIGN_CENTER,
		LINE_ALIGN_STRETCH,
	};
	enum PARA_ALIGN
	{
		PARA_ALIGN_START,
		PARA_ALIGN_END,
		PARA_ALIGN_CENTER,
		PARA_ALIGN_STRETCH,
	};
	struct Axis {
		AXIS_DIR direction;
		AXIS_SPACE space;

	};
	struct AxisProps
	{
		AXIS_MAJOR majorAxis;

		Axis major;
		Axis bi;
	};
	struct CharMetrics
	{
		Math3D::Vector2 offset;
		Math3D::Vector2 advance;
		float ascender, decender;
	};
	struct PharaseMetrics
	{

	};
	struct BlockMetrics
	{
		Math3D::float4 margin;
		Math3D::float4 border;
		Math3D::float4 padding;
		Math3D::float4 content;
	};

	struct LineMetrics
	{
		Math3D::Vector2 size;
		Math3D::Vector2 baseline;
		float ascender, decender;
	};

	struct BlockProps
	{
		Math3D::Vector2 minSize;
		Math3D::Vector2 maxSize;
		Math3D::Vector2 properSize;
		float lineGap;
	};
	struct GlyphMetrics
	{
		Math3D::float2 size;
		struct SpecialMetrics
		{
			Math3D::float2 offset;
			float advance;
		};
		SpecialMetrics vertical;
		SpecialMetrics horizontal;

	};
	struct SizeI
	{
		int x = 0, y = 0;
		SizeI() = default;
		SizeI(int x,int y) 
		{
			this->x = x;
			this->y = y;
		};
		SizeI(Math3D::Vector2& v)
		{
			x = v.x;
			y = v.y;
		}
	};

	struct Size :public Math3D::Vector2
	{
		Size() 
		{
			x = 0;
			y = 0;
		}
		Size(float x, float y)
		{
			this->x = x;
			this->y = y;
		}
		Size(SizeI& s)
		{
			x = s.x;
			y = s.y;
		}
	};

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
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
		int width()
		{
			return right - left;
		}
		int height()
		{
			return bottom - top;
		}
		RectangleI() 
		{
		};
		RectangleI(int l, int t, int r, int b)
		{
			left = l;
			top = t;
			right = r;
			bottom = b;
		}
	};
	struct Rectangle : public Math3D::Quaternion
	{

	};
}