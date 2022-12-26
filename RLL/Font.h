#pragma once
#include "Sharp.h"
#include <vector>
#include <map>
class FontFamily;
class FontFallbacks;
class FontFace;
struct _FontFace;

enum FontStyle
{
	None = 0,
	Italic = 1,
};
enum FontWeight
{
	Thin = 100,
	Lighter = 200,
	Light = 300,
	Regular = 400,
	Medium = 500,
	SemiBold = 600,
	Bold = 700,
	Bolder = 800,
	Black = 900
};
class FontFormat
{
	struct GlyphDetail
	{
		void* metrics;
		float uv[3]; //[0],[1] = u,v; [2] = vtex index.
	};
	FontFace* face;
	float size;
	//textures that on gpu
	std::map<char32_t, GlyphDetail> glyphs;
	std::vector<void*> textures;//d3d12: tex descriptors, heap

};
class FontFace
{
private:
	FontFamily* family;
	Sharp::String name;
	_FontFace* data;
	int style;
	int weight;
	float size = 16;

	void RenderGlyph(char32_t code,void* bitmap);

};

class FontFamily
{
private:
	Sharp::String name;
	FontFace* fonts;
public:
	FontFace* GetFace(FontStyle style = FontStyle::None, FontWeight weight = FontWeight::Regular, float size = 16);
};

class FontFallbacks
{
	std::vector<FontFamily*> families;

};

