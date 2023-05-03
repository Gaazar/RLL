#pragma once
#include "TextInterfaces.h"
#include "freetype/freetype.h"
#include "freetype/ftbitmap.h"


class FTFace;
class FTFactory :public RLL::IFontFactory
{
	friend class FTFace;
	RLL::IPaintDevice* device;
	FT_Library lib;
	RLL::ISVG* notdef;
	void Dispose() {};
public:
	FTFactory(RLL::IPaintDevice*);
	RLL::IFontFace* LoadFromFile(char*);

};
class FTFace : public RLL::IFontFace
{
public:
	friend class FTFactory;
	FTFactory* factory;
	FT_Face face;
	FT_Palette_Data palinfo;
	RLL::IBrush** brushes;
	RLL::ISVG** cache_svg;

	void Load();
	void Dispose() { NOIMPL; };
	void GetMetrics(RLL::GlyphMetrics* met);

public:
	void SetLevel() { NOIMPL; };
	uint32_t GetCodepoint(char32_t unicode);
	RLL::IGeometry* GetPlainGlyph(char32_t unicode, RLL::GlyphMetrics* metrics = nullptr);
	RLL::IGeometry* GetPlainGlyph(uint32_t codepoint, RLL::GlyphMetrics* metrics = nullptr);
	RLL::ISVG* GetGlyph(char32_t unicode, RLL::GlyphMetrics* metrics);
	RLL::ISVG* GetGlyph(uint32_t codepoint, RLL::GlyphMetrics* metrics);
};
