#include "TextInterfaces.h"
#include "fi_ft.h"

#include "unicode/msvclib.h"
#include "unicode/ucnv.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"
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
	contentSize = sz;
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
bool bidi_ltr(LANG_DIRECTION d)
{
	return (d == LANG_DIRECTION_BT_LR || LANG_DIRECTION_TB_LR || LANG_DIRECTION_LR_TB || LANG_DIRECTION_LR_BT);
}
void TextLayout::Break()
{
	std::vector<BreakPart> fs_bidi;
	std::vector<BreakPart> fs_ucs;
	std::vector<BreakPart> fs_script;
	//hb_font_t* hbf = hb_ft_font_create_referenced(ftf->face);
	BreakPart fs;
	UErrorCode uec = U_ZERO_ERROR;
	UChar* uTex = (UChar*)text;
	//break bidi
	{
		UBiDi* bidi = ubidi_open();
		UBiDiLevel bidiReq = bidi_ltr(direction) ? UBIDI_DEFAULT_LTR : UBIDI_DEFAULT_RTL;
		ubidi_setPara(bidi, uTex, textLen, bidiReq, NULL, &uec);
		if (U_SUCCESS(uec)) {
			//int paraDir = ubidi_getParaLevel(bidi);
			size_t rc = ubidi_countRuns(bidi, &uec);
			for (size_t i = 0; i < size_t(rc); ++i)
			{
				int32_t startRun = -1;
				int32_t lengthRun = -1;
				UBiDiDirection runDir = ubidi_getVisualRun(bidi, i, &startRun, &lengthRun);
				fs.ucs = uTex + startRun;
				fs.len = lengthRun;
				fs.rtl = runDir == UBIDI_RTL;
				fs_bidi.push_back(fs);
			}
		}
		ubidi_close(bidi);
	}
	//break font styles
	for (auto& s : fs_bidi)
	{
		fs = s;
		auto j = s.ucs - uTex;
		int32_t b = 0;
		FTFace* ftf = GetFTFace(ffaces[j], ((char16_t*)s.ucs)[j]);
		for (int i = 0; i < s.len; i++)
		{
			auto n = j + i;
			if (ftf != GetFTFace(ffaces[n], ((char16_t*)s.ucs)[i]))
			{
				if (i - b > 0)
				{
					fs.ucs = &(s.ucs)[b];
					fs.len = i - b;
					fs.face = ftf;
					fs_ucs.push_back(fs);
				}
				b = i;
				ftf = GetFTFace(ffaces[n], ((char16_t*)s.ucs)[i]);
			}
		}
		fs.ucs = &(s.ucs)[b];
		fs.len = s.len - b;
		fs.face = ftf;
		fs_ucs.push_back(fs);
	}

	for (auto& i : fs_ucs)
	{
		fs = i;
		int b = 0;
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
		fs = i;
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
				parts.push_back(fs);
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
	UErrorCode uec = U_ZERO_ERROR;
	auto dpi = RLL::GetScale() * 96;
	for (auto& p : parts)
	{
		uec = U_ZERO_ERROR;
		if (!U_SUCCESS(uec))
		{
			assert(0);
		}
		auto f = dynamic_cast<FTFace*>(p.face);
		FT_Set_Char_Size(f->face, 72 * 64, 0, 32, 32);
		Math3D::Vector2 scale(
			f->face->size->metrics.x_scale / 65536.f * f->face->units_per_EM,
			f->face->size->metrics.y_scale / 65536.f * f->face->units_per_EM);

		auto hbf = hb_ft_font_create_referenced(f->face);
		hb_buffer_t* buf = hb_buffer_create();;

		//u_charTo
		//UChar32 c32;
		//u_strToUTF32(&c32, 1, nullptr, &s.ucs[startRun], 1, &uec);
		//auto sc = uscript_getScript(c32, &uec);
		//printf("Processing Bidi Run = %d -- run-start = %d, run-len = %d, isRTL = %d\n",
		//	i, startRun, lengthRun, isRTL);
		hb_buffer_add_utf16(buf, (uint16_t*)p.ucs, p.len, 0, -1);

		hb_buffer_set_script(buf, hb_icu_script_to_script(p.script));
		hb_buffer_set_direction(buf, p.rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
		hb_buffer_set_language(buf, hb_language_from_string("en", -1));

		hb_shape(hbf, buf, NULL, 0);

		unsigned int glyph_count;
		hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
		hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
		//TimeCheck("Getglyf start");
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
			auto sg = p.face->GetGlyph(glyphid, &gmet);
			Vector2 adv(x_advance, y_advance);
			auto gsc = Vector2(fsz * dpi.x / 72.f, fsz * dpi.y / 72.f);
			p.size.y = gsc.y;
			p.size.x += adv.x / scale.x * gsc.x; //horizontal layout only
			if (gmet.size.y * gsc.y > szMax)
				szMax = gmet.size.y * gsc.y;
			p.glyfs.push_back(sg);
			//bp.glyfAdvance.push_back({ xa, ya });
			p.glyfOffset.push_back({ (curs.x + x_offset) / scale.x * gsc.x ,(curs.y + y_offset) / scale.y * gsc.y });
			curs += adv;
			p.glyfScale.push_back(gsc);
		}
		hb_buffer_destroy(buf);
		p.size.y = szMax - szMin;

		hb_font_destroy(hbf);
	}
	//ubidi_close(bidi);
}

void TextLayout::Place(ISVGBuilder* sb)
{
	struct BIDIPart
	{
		std::vector<BreakPart> parts;
		bool rtl = false;
	};
	struct LinePart
	{
		std::vector<BreakPart> parts;
		std::vector<BIDIPart> bidis;
		Size size;
	};
	std::vector<LinePart> lines;
	LinePart cLine;
	//horizontal only

	//calculate line metrics
	{
		for (auto& i : parts)
		{
			if (cLine.size.x + i.size.x > contentSize.x)
			{
				if (cLine.parts.size() > 0)
				{
					lines.push_back(cLine);
					cLine.parts.clear();
					cLine.size = Size(0, 0);
					//continue;
				}
			}
			cLine.parts.push_back(i);
			cLine.size.x += i.size.x;
			
			if (i.size.y > cLine.size.y)
			{
				cLine.size.y = i.size.y;
			}
		}
		if (cLine.parts.size() > 0)
		{
			lines.push_back(cLine);
		}
	}

	//bidi lines
	{
		for (auto& l : lines)
		{
			bool inv = false;
			//fix two terminal of the line's COMMON_SCRIPT bidi info
			for (auto k = 0; k < l.parts.size(); k++)
			{
				if (l.parts[k].script == USCRIPT_COMMON)
				{
					l.parts[k].rtl = false;
				}
				else break;
			}
			for (auto k = l.parts.size() - 1; k >= 0; k--)
			{
				if (l.parts[k].script == USCRIPT_COMMON)
				{
					l.parts[k].rtl = false;
				}
				else break;
			}
			//TODO: combine below two for, can it?
			BIDIPart bdp;
			for (auto i = 0; i < l.parts.size(); i++)
			{
				if (l.parts[i].rtl != inv)
				{
					if (bdp.parts.size() > 0)
					{
						l.bidis.push_back(bdp);
						bdp.parts.clear();
					}
					bdp.rtl = l.parts[i].rtl;
				}
				inv = l.parts[i].rtl;
				bdp.parts.push_back(l.parts[i]);

			}
			if (bdp.parts.size() > 0)
				l.bidis.push_back(bdp);
			bdp.parts.clear();
			l.parts.clear();
			for (auto& i : l.bidis)
			{
				if (i.rtl)
				{
					for (auto it = i.parts.rbegin(); it != i.parts.rend(); )
					{
						l.parts.push_back(*it);
						++it;
					}
				}
				else
				{
					for (auto it = i.parts.begin(); it != i.parts.end(); )
					{
						l.parts.push_back(*it);
						++it;
					}
				}
			}
		}
	}

	float2 cursor(0, 0);
	for (int i = 0; i < lines.size(); i++)
	{
		cursor.y += lines[i].size.y * 1.2f;
		cursor.x = 0;
		for (auto& n : lines[i].parts)
		{
			for (int j = 0; j < n.glyfs.size(); j++)
			{
				if (n.glyfs[j])
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
void TextLayout::SetParagraphAlign(PARA_ALIGN axis)
{
	NOIMPL;
}
void TextLayout::SetLineAlign(LINE_ALIGN axis)
{
	NOIMPL;
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