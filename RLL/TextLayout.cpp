#include "TextInterfaces.h"
#include "fi_ft.h"

#include "unicode/msvclib.h"
#include "unicode/ucnv.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"
#include "unicode/uscript.h"
#include "unicode/brkiter.h"

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ft.h"
#include "harfbuzz/hb-icu.h"

#pragma comment(lib, "harfbuzz.lib")
#pragma comment(lib, "harfbuzz-icu.lib")

using namespace RLL;
using namespace icu;
using namespace Math3D;

TextLayout::TextLayout(wchar_t* text, RLL::Size sz, IFontFace* fface)
{
	UErrorCode uec = U_ZERO_ERROR;
	rectSize = sz;
	textLen = wcslen(text);
	this->text = new UChar[textLen + 1];
	u_strFromWCS((UChar*)this->text, textLen + 1, &textLen, text, textLen, &uec);

	ffaces.Set(0, -1, fface);
	fsizes.Set(0, -1, 12);
	//fsizes.Set(2, 8, 16);

}
void TextLayout::SetFontFace(RLL::IFontFace* ff, int32_t begin, int32_t len)
{

}
FTFace* GetFTFace(IFontFace* f, char16_t c)
{
	FTFace* ftf = dynamic_cast<FTFace*>(f);
	if (!ftf)
	{
		auto fs = dynamic_cast<IFontStack*>(f);
		ftf = (FTFace*)fs->GetFace(c);
	}
	return ftf;
}
void TextLayout::Break()
{
	split.clear();
	std::vector<FontSplit> fs_ucs;
	FTFace* ftf = GetFTFace(ffaces[-1], ((char16_t*)text)[0]);
	int32_t b = 0;
	//hb_font_t* hbf = hb_ft_font_create_referenced(ftf->face);
	FontSplit fs;
	for (int i = 0; i < textLen; i++)
	{
		if (ftf != GetFTFace(ffaces[i], ((char16_t*)text)[i]))
		{
			if (i - b > 0)
			{
				fs.ucs = &((UChar*)text)[b];
				fs.len = i - b;
				fs.face = ftf;
				fs_ucs.push_back(fs);
			}
			b = i;
			ftf = GetFTFace(ffaces[i], ((char16_t*)text)[i]);
		}
	}
	fs.ucs = &((UChar*)text)[b];
	fs.len = textLen - b;
	fs.face = ftf;
	fs_ucs.push_back(fs);

	UErrorCode uec = U_ZERO_ERROR;
	std::vector<FontSplit> fs_script;
	for (auto& i : fs_ucs)
	{
		fs = i;
		b = 0;
		fs.script = USCRIPT_COMMON;
		UChar32* uc32 = new UChar32[i.len];
		u_strToUTF32(uc32, i.len, nullptr, i.ucs, i.len, &uec);
		for (auto n = 0; n < i.len; n++)
		{
			//UChar32 c32 = 0;
			//u_strToUTF32(&c32, 1, nullptr, &i.ucs[n], 1, &uec);
			uec = U_ZERO_ERROR;
			auto sc = uscript_getScript(uc32[n], &uec);
			if (!U_SUCCESS(uec))
			{
				sc = USCRIPT_COMMON;
			}
			uec = U_ZERO_ERROR;
			if (fs.script != sc)
			{
				if (n - b > 0)
				{
					fs.len = n - b;
					fs_script.push_back(fs);
					fs.ucs = i.ucs + n;
					b = n;
				}
				fs.script = sc;
			}
		}
		delete uc32;
		fs.ucs = &(i.ucs)[b];
		fs.len = i.len - b;
		fs_script.push_back(fs);
	}



	UBiDi* bidi = ubidi_open();
	UBiDiLevel bidiReq = UBIDI_DEFAULT_LTR;
	//std::vector<FontSplit> brk_ucs;
	auto brki = icu::BreakIterator::createLineInstance(icu::Locale::getDefault(), uec);

	for (auto& i : fs_script)
	{
		auto utx = utext_openUChars(nullptr, i.ucs, i.len, &uec);
		brki->setText(utx, uec);
		int32_t p = brki->first();
		int32_t p_l = p;
		while (p != icu::BreakIterator::DONE) {
			//printf("Boundary at position %d\n", p);
			if (p_l != p)
			{
				int32_t wlen = p - p_l;
				//UChar* word = new UChar[p - p_l + 1]{ 0 };
				//utext_extract(utx, p_l, p, word, p - p_l + 1, &uec);
				fs.face = i.face;
				fs.len = wlen;
				fs.ucs = &i.ucs[p_l];
				fs.script = i.script;
				split.push_back(fs);
			}
			p_l = p;
			p = brki->next();
		}

		utext_close(utx);
	}
	delete brki;

}
void TextLayout::Metrics()
{
	parts.clear();
	UErrorCode uec = U_ZERO_ERROR;
	UBiDi* bidi = ubidi_open();
	UBiDiLevel bidiReq = UBIDI_DEFAULT_LTR;
	BreakPart bp;
	auto dpi = RLL::GetScale() * 96;
	for (auto& s : split)
	{
		uec = U_ZERO_ERROR;
		ubidi_setPara(bidi, s.ucs, s.len, bidiReq, NULL, &uec);
		if (!U_SUCCESS(uec))
		{
			assert(0);
		}
		auto f = dynamic_cast<FTFace*>(s.face);
		FT_Set_Char_Size(f->face, 72 * 64, 0, 32, 32);
		Math3D::Vector2 scale(
			f->face->size->metrics.x_scale / 65536.f * f->face->units_per_EM,
			f->face->size->metrics.y_scale / 65536.f * f->face->units_per_EM);

		size_t rc = ubidi_countRuns(bidi, &uec);
		auto hbf = hb_ft_font_create_referenced(f->face);
		for (size_t i = 0; i < size_t(rc); ++i) {

			hb_buffer_t* buf = hb_buffer_create();;

			int32_t startRun = -1;
			int32_t lengthRun = -1;
			UBiDiDirection runDir = ubidi_getVisualRun(bidi, i, &startRun, &lengthRun);
			//u_charTo
			//UChar32 c32;
			//u_strToUTF32(&c32, 1, nullptr, &s.ucs[startRun], 1, &uec);
			//auto sc = uscript_getScript(c32, &uec);
			bool isRTL = (runDir == UBIDI_RTL);
			//printf("Processing Bidi Run = %d -- run-start = %d, run-len = %d, isRTL = %d\n",
			//	i, startRun, lengthRun, isRTL);
			hb_buffer_add_utf16(buf, (uint16_t*)s.ucs, s.len, startRun, lengthRun);

			hb_buffer_set_script(buf, hb_icu_script_to_script(s.script));
			hb_buffer_set_direction(buf, isRTL ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
			hb_buffer_set_language(buf, hb_language_from_string("en", -1));

			hb_shape(hbf, buf, NULL, 0);

			unsigned int glyph_count;
			hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
			hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
			//TimeCheck("Getglyf start");
			bp.size = { 0,0 };
			bp.pxpem = { 0,0 };
			bp.glyfs.clear();
			bp.glyfScale.clear();
			bp.glyfOffset.clear();
			bp.script = s.script;
			bp.rtl = isRTL;
			float szMax = 0; float szMin = 0;
			Vector2 curs(0, 0);
			for (unsigned int i = 0; i < glyph_count; i++) {
				hb_codepoint_t glyphid = glyph_info[i].codepoint;
				hb_position_t x_offset = glyph_pos[i].x_offset;
				hb_position_t y_offset = glyph_pos[i].y_offset;
				hb_position_t x_advance = glyph_pos[i].x_advance;
				hb_position_t y_advance = glyph_pos[i].y_advance;
				/* draw_glyph(glyphid, cursor_x + x_offset, cursor_y + y_offset); */
				//std::cout << x_advance / 64.f << "\t" << y_advance / 64.f << std::endl;
				//TODO:
				/*
				* make size, scale, every font size...;
				*
				*/
				auto fsz = fsizes[glyph_info[i].cluster];
				GlyphMetrics gmet;
				auto sg = s.face->GetGlyph(glyphid, &gmet);
				Vector2 adv(x_advance, y_advance);
				auto gsc = Vector2(fsz * dpi.x / 72.f, fsz * dpi.y / 72.f);

				bp.size.x += gmet.horizontal.advance * gsc.x; //horizontal layout only
				if (gmet.size.y * gsc.x > szMax)
					szMax = gmet.size.y * gsc.x;
				if (gsc.x > bp.pxpem.x)
					bp.pxpem.x = gsc.x;
				if (gsc.y > bp.pxpem.y)
					bp.pxpem.y = gsc.y;
				bp.glyfs.push_back(sg);
				//bp.glyfAdvance.push_back({ xa, ya });
				bp.glyfOffset.push_back({ (curs.x + x_offset) / scale.x * gsc.x ,(curs.y + y_offset) / scale.y * gsc.y });
				curs += adv;
				bp.glyfScale.push_back(gsc);
			}
			hb_buffer_destroy(buf);
			bp.size.y = szMax - szMin;
			parts.push_back(bp);
		}
		hb_font_destroy(hbf);
	}
	ubidi_close(bidi);
}

void TextLayout::Place(ISVGBuilder* sb)
{
	struct LinePart
	{
		std::vector<BreakPart> parts;
		Size size;
		float2 pxpem;
	};
	std::vector<LinePart> lines;
	LinePart cLine;
	//horizontal only
	//notmalize direction
	std::vector<BreakPart> nparts;
	{
		int joiner = 0;
		bool inv = false;
		int b = 0;
		int e = 0;
		UScriptCode lsc = USCRIPT_INVALID_CODE;
		for (auto i = 0; i < parts.size(); i++)
		{
			//if (lsc == USCRIPT_COMMON && i > 0)
			//{
			//	parts[i - 1].invDirection = parts[i].invDirection;
			//}
			//lsc = parts[i].script;

			if (parts[i].script == USCRIPT_COMMON)
			{
				bool befo = false; //is before invDirection
				bool afte = false;
				auto n = i - 1;
				while (n >= 0)
				{
					if (parts[n].script != USCRIPT_COMMON)
					{
						befo = parts[n].rtl;
						break;
					}
					n--;
				}
				n = i + 1;
				while (n < parts.size())
				{
					if (parts[n].script != USCRIPT_COMMON)
					{
						afte = parts[n].rtl;
						break;
					}
					n++;
				}
				if (afte == befo)
					parts[i].rtl = afte;
				else
					parts[i].rtl = false;
			}
		}
		for (auto i = 0; i < parts.size(); i++)
		{
			if (parts[i].rtl)
			{
				if (inv == false)
				{
					b = i;
				}
				nparts.insert(nparts.begin() + b, parts[i]);
				inv = true;
			}
			else
			{
				nparts.push_back(parts[i]);
				inv = false;
			}
		}
	}
	//calculate line metrics
	for (auto& i : nparts)
	{
		if (cLine.size.x + i.size.x > rectSize.x)
		{
			if (cLine.parts.size() > 0)
			{
				lines.push_back(cLine);
				cLine.parts.clear();
				cLine.size = Size(0, 0);
				cLine.pxpem = Vector2(0, 0);
				//continue;
			}
		}
		cLine.parts.push_back(i);
		cLine.size.x += i.size.x;
		if (i.pxpem.x > cLine.pxpem.x)
			cLine.pxpem.x = i.pxpem.x;
		if (i.pxpem.y > cLine.pxpem.y)
			cLine.pxpem.y = i.pxpem.y;

		if (i.size.y > cLine.size.y)
		{
			cLine.size.y = i.size.y;
		}
	}
	if (cLine.parts.size() > 0)
	{
		lines.push_back(cLine);
	}
	float2 cursor(0, 0);
	for (int i = 0; i < lines.size(); i++)
	{
		cursor.y += lines[i].pxpem.y * 1.2f;
		cursor.x = 0;
		for (auto& n : lines[i].parts)
		{
			for (int j = 0; j < n.glyfs.size(); j++)
			{
				sb->Push(n.glyfs[j], &Matrix4x4::TRS(
					Vector3(cursor + n.glyfOffset[j]), Quaternion::identity(),
					Vector3(n.glyfScale[j].x, n.glyfScale[j].y, 1)));
				//auto o = cursor + n.glyfOffset[j];
				//sb->Push(n.glyfs[j],
				//	&(Matrix4x4::Scaling({ n.glyfScale[j].x, n.glyfScale[j].y, 1 }) * 
				//	Matrix4x4::Translation({ j * 16.f,i * 16.f,0 }))
				//);
			}
			cursor.x += n.size.x;
		}
	}
}
void TextLayout::SetParagraphAlign(PARA_ALIGN axis, PARA_ALIGN biAxis)
{
	NOIMPL;
}
void TextLayout::SetLineAlign(LINE_ALIGN axis, LINE_ALIGN biAxis)
{
	lineAlign.x = axis;
	lineAlign.y = biAxis;
}
RLL::ISVG* TextLayout::Commit(RLL::IPaintDevice* dvc)
{
	Break();
	Metrics();
	auto sb = dvc->CreateSVGBuilder();
	Place(sb);
	auto svg = sb->Commit();
	sb->Release();
	NOIMPL; //TODO: Layout
	return svg;
}
void TextLayout::Dispose()
{
	delete[] text;
	NOIMPL;
}

void RLL::TextLayoutInit() //abandoned
{
	//return;
	hb_buffer_t* buf;
	buf = hb_buffer_create();
	hb_buffer_add_utf16(buf, (uint16_t*)L"اللغة العربية", -1, 0, -1);
	hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
	hb_buffer_set_script(buf, HB_SCRIPT_ARABIC);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));

	hb_blob_t* blob = hb_blob_create_from_file("c:/windows/fonts/arial.ttf"); /* or hb_blob_create_from_file_or_fail() */
	hb_face_t* face = hb_face_create(blob, 0);
	hb_font_t* font = hb_font_create(face);

	hb_shape(font, buf, NULL, 0);

	hb_buffer_destroy(buf);
	hb_font_destroy(font);
	hb_face_destroy(face);
	hb_blob_destroy(blob);
}