#pragma once
#include "math3d/Math3Df.h"
struct CharMetrics
{
	Math3D::Vector2 offset;
	Math3D::Vector2 size;
	Math3D::Vector2 advance;
	float ascender, decender;
};

struct BlockMetrics
{
	Math3D::Vector4 margin;
	Math3D::Vector4 border;
	Math3D::Vector4 padding;
	Math3D::Vector4 content;
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
};