#include "fi_ft.h"
#include "freetype/freetype.h"
#include "freetype/ftoutln.h"
#include "freetype/ftbbox.h"
#include "freetype/ftbitmap.h"

#define NO_TIMECHECKER
#include "perf_util.h"

#pragma comment(lib, "freetype.lib")
using namespace RLL;
using namespace Math3D;

FT_Outline_Funcs dcomp_funcs;

struct DecompParam
{
	float scale;
	int index = 0;
	IGeometryBuilder* gb;
};
int dcomp_qbezier(const FT_Vector* control, const FT_Vector* to, void* p)
{
	DecompParam* bd = (DecompParam*)p;
	auto gb = bd->gb;
	auto s = bd->scale;
	if (bd->index == 0)
	{
		gb->Begin({ 0,0 });
	}
	gb->QuadraticTo(
		{ (float)control->x * s ,-(float)control->y * s },
		{ (float)to->x * s ,-(float)to->y * s });
	bd->index++;
	return 0;
}
int dcomp_mvto(const FT_Vector* pos, void* p)
{
	//std::cout << "mv\t" << fp(pos->x) << "," << fp(pos->y) << std::endl;
	DecompParam* bd = (DecompParam*)p;
	auto gb = bd->gb;
	auto s = bd->scale;
	if (bd->index != 0)
	{
		gb->End(false);
	}
	gb->Begin({ (float)pos->x * s, -(float)pos->y * s });
	bd->index++;
	return 0;
}
int dcomp_lineto(const FT_Vector* to, void* p)
{
	//std::cout << "li\t" << fp(to->x) << "," << fp(to->y) << std::endl;
	DecompParam* bd = (DecompParam*)p;
	auto gb = bd->gb;
	auto s = bd->scale;
	if (bd->index == 0)
	{
		gb->Begin({ 0,0 });
	}
	gb->LineTo({ (float)to->x * s ,-(float)to->y * s });
	bd->index++;
	return 0;
}
int dcomp_cbezier(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* p)
{
	DecompParam* bd = (DecompParam*)p;
	auto gb = bd->gb;
	auto s = bd->scale;
	if (bd->index == 0)
	{
		gb->Begin({ 0,0 });
	}
	gb->CubicTo({ (float)c1->x * s ,-(float)c1->y * s },
		{ (float)c2->x * s ,-(float)c2->y * s },
		{ (float)to->x * s ,-(float)to->y * s }
	);
	bd->index++;
	return 0;
}

IFontFactory* RLL::CreateFontFactory(IPaintDevice* device)
{
	dcomp_funcs.conic_to = dcomp_qbezier;
	dcomp_funcs.cubic_to = dcomp_cbezier;
	dcomp_funcs.move_to = dcomp_mvto;
	dcomp_funcs.line_to = dcomp_lineto;
	dcomp_funcs.delta = 0;
	dcomp_funcs.shift = 0;



	return new FTFactory(device);
}
FTFactory::FTFactory(IPaintDevice* d)
{
	FT_Init_FreeType(&lib);
	device = d;
	notdef = nullptr;
	//auto sb = d->CreateSVGBuilder();
	//auto gb = d->CreateGeometryBuilder();
	//gb->Rectangle({ 0,-1 }, { 1,0 });
	//gb->Rectangle({ 0.1,-0.9 }, { 0.9,-0.1 }, true);
	//sb->Push(gb->Fill());
	//notdef = sb->Commit();
	//gb->Release();
	//sb->Release();
}


RLL::IFontFace* FTFactory::LoadFromFile(char* path)
{
	auto fc = new FTFace();
	fc->factory = this;
	if (FT_New_Face(lib, path, 0, &fc->face)) //seguiemj.ttf arial.ttf
	{
		delete fc;
		SetError("Load font file error.");
		return nullptr;
	}
	fc->Load();
	return fc;

}
void FTFace::Load()
{
	FT_Palette_Data palinfo;
	FT_Palette_Data_Get(face, &palinfo);
	brushes = new IBrush * [palinfo.num_palette_entries]{ nullptr };
	cache_svg = new ISVG * [face->num_glyphs]{ nullptr };
}
uint32_t FTFace::GetCodepoint(char32_t unicode)
{
	return FT_Get_Char_Index(face, unicode);
}

RLL::IGeometry* FTFace::GetPlainGlyph(char32_t unicode)
{
	auto glyph_index = FT_Get_Char_Index(face, unicode);
	if (glyph_index == 0) return nullptr;
	return GetPlainGlyph(glyph_index);
}
RLL::IGeometry* FTFace::GetPlainGlyph(uint32_t codepoint)
{
	auto hr = FT_Load_Glyph(face, codepoint, FT_LOAD_COLOR | FT_LOAD_NO_SCALE);
	auto& gol = face->glyph->outline;
	DecompParam dp;
	dp.gb = factory->device->CreateGeometryBuilder();
	dp.scale = 1.f / face->units_per_EM;
	dp.index = 0;
	FT_Outline_Decompose(&gol, &dcomp_funcs, &dp);
	dp.gb->End(false);
	auto res = dp.gb->Fill();
	dp.gb->Release();
	return res;

}

RLL::ISVG* FTFace::GetGlyph(char32_t unicode)
{
	auto glyph_index = FT_Get_Char_Index(face, unicode);
	return GetGlyph(glyph_index);
}
RLL::ISVG* FTFace::GetGlyph(uint32_t codepoint)
{
	TimeCheck("GetGlyph Begin");
	auto glyph_index = codepoint;
	if (cache_svg[glyph_index] != nullptr)
	{
		TimeCheck("GetGlyph done. Cache hit.");
		TimeCheckSum();
		return cache_svg[glyph_index];
	}
	DecompParam dp;
	ISVGBuilder* sb = factory->device->CreateSVGBuilder();
	dp.gb = factory->device->CreateGeometryBuilder();
	dp.scale = 1.f / face->units_per_EM;
	IBrush* br = nullptr;
	FT_Color* palcol;
	FT_LayerIterator itr;
	itr.p = nullptr;
	itr.num_layers = 0;
	FT_UInt glyf_i;
	FT_UInt glyf_c;
	FT_Palette_Select(face, 0, &palcol);
	ISVG* res = nullptr;
	while (FT_Get_Color_Glyph_Layer(face, glyph_index, &glyf_i, &glyf_c, &itr))
	{
		FT_Load_Glyph(face, glyf_i, FT_LOAD_COLOR | FT_LOAD_DEFAULT | FT_LOAD_NO_SCALE);
		auto& gol = face->glyph->outline;
		dp.gb->Reset();
		dp.index = 0;
		FT_Outline_Decompose(&gol, &dcomp_funcs, &dp);
		dp.gb->End(false);
		if (glyf_c != 0xff)
		{
			auto col = palcol[glyf_c];
			if (brushes[glyf_c] == nullptr)
			{
				brushes[glyf_c] = factory->device->CreateSolidColorBrush({ col.red / 255.f,col.green / 255.f,col.blue / 255.f,0 });
			}
			br = brushes[glyf_c];
		}
		else
		{
			br = nullptr;
		}
		auto geo = dp.gb->Fill();
		sb->Push(geo, br);
	}
	if (!itr.num_layers)
	{
		//TimeCheck("GetGlyph no layered entry.");
		auto hr = FT_Load_Glyph(face, glyph_index, FT_LOAD_COLOR | FT_LOAD_NO_SCALE);
		auto& gol = face->glyph->outline;
		if (gol.n_contours == 0)
		{
			if (glyph_index == 0)
				res = factory->notdef;
			else
				res = nullptr;
		}
		else
		{
			TimeCheck("GetGlyph decompos.");
			dp.index = 0;
			dp.gb->Reset();
			FT_Outline_Decompose(&gol, &dcomp_funcs, &dp);
			dp.gb->End(false);
			TimeCheck("GetGlyph decompos done. Geom build begn");
			auto geo = dp.gb->Fill();
			TimeCheck("GetGlyph geom built. push");
			if (geo)
				sb->Push(geo, nullptr);
			TimeCheck("GetGlyph svg commit begin.");
			res = sb->Commit();
			TimeCheck("GetGlyph svg commited.");
		}
	}
	else
	{
		res = sb->Commit();
		//TimeCheck("GetGlyph layered svg commited.");
	}
	dp.gb->Release();
	sb->Release();
	cache_svg[glyph_index] = res;
	TimeCheck("GetGlyph done.");
	TimeCheckSum();
	return  res;
}
