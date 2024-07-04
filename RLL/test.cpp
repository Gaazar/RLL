#include "rlltest.h"
#include "fi_ft.h"

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ft.h"
#include "harfbuzz/hb-icu.h"

#include "unicode/ucnv.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"
#include "unicode/uscript.h"
#include "unicode/brkiter.h"

#include "perf_util.h"

#pragma comment(lib, "harfbuzz.lib")
#pragma comment(lib, "harfbuzz-icu.lib")

#pragma comment(lib, "icuuc.lib")
#pragma comment(lib, "icutu.lib")
#pragma comment(lib, "icuio.lib")
#pragma comment(lib, "icuin.lib")
#pragma comment(lib, "icudt.lib")


using namespace Math3D;
using namespace RLL;

void hb_init()
{
}
void ft_hb_destroy(void*)
{

}

RLL::ISVG* hb_test(RLL::IFontFace* ff, RLL::ISVGBuilder* sb, wchar_t* tex)
{
	sb->Reset();
	auto f = (FTFace*)ff;
	FT_Set_Char_Size(f->face, 72 * 64, 0, 32, 32);
	//scale / 65536.f * u/em / 64.f = px
	//render unit = 1/(u/em)

	Math3D::Vector2 scale(
		f->face->size->metrics.x_scale / 65536.f * f->face->units_per_EM,
		f->face->size->metrics.y_scale / 65536.f * f->face->units_per_EM);
	auto t = scale * 2048;

	hb_position_t cursor_x = 0;
	hb_position_t cursor_y = 0;
	//hb_buffer_guess_segment_properties(buf);
	auto font = hb_ft_font_create_referenced(f->face);
	UChar* ucs = new UChar[wcslen(tex) + 1];
	int32_t tsl = 0;
	UErrorCode uec = U_ZERO_ERROR;
	u_strFromWCS(ucs, wcslen(tex) + 1, &tsl, tex, -1, &uec);

	auto utx = utext_openUChars(nullptr, ucs, tsl, &uec);


	auto brki = icu::BreakIterator::createLineInstance(icu::Locale::getDefault(), uec);
	brki->setText(utx, uec);

	int32_t p = brki->first();
	setlocale(LC_ALL, "");
	int32_t p_l = p;
	printf("%ls\n", tex);
	while (p != icu::BreakIterator::DONE) {
		//printf("Boundary at position %d\n", p);
		if (p_l != p)
		{
			UChar* word = new UChar[p - p_l + 1]{0};
			utext_extract(utx, p_l, p, word, 999999, &uec);
			printf("%ls|", word);
			delete[] word;
		}
		p_l = p;
		p = brki->next();
	}
	printf("\n");
	delete brki;
	utext_close(utx);

	UBiDi* bidi = ubidi_open();
	UBiDiLevel bidiReq = UBIDI_DEFAULT_LTR;
	int stringLen = wcslen(tex);
	if (bidi) {
		UErrorCode status = U_ZERO_ERROR;
		ubidi_setPara(bidi, ucs, stringLen, bidiReq, NULL, &status);

		if (U_SUCCESS(status)) {
			//int paraDir = ubidi_getParaLevel(bidi);
			size_t rc = ubidi_countRuns(bidi, &status);
			for (size_t i = 0; i < size_t(rc); ++i) {

				hb_buffer_t* buf = hb_buffer_create();;

				int32_t startRun = -1;
				int32_t lengthRun = -1;
				UBiDiDirection runDir = ubidi_getVisualRun(bidi, i, &startRun, &lengthRun);
				//u_charTo
				UChar32 c32;
				u_strToUTF32(&c32, 1, nullptr, &ucs[startRun], 1, &status);
				auto sc = uscript_getScript(c32, &status);
				bool isRTL = (runDir == UBIDI_RTL);
				//printf("Processing Bidi Run = %d -- run-start = %d, run-len = %d, isRTL = %d\n",
				//	i, startRun, lengthRun, isRTL);
				hb_buffer_add_utf16(buf, (uint16_t*)ucs, -1, startRun, lengthRun);

				hb_buffer_set_direction(buf, isRTL ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
				hb_buffer_set_script(buf, hb_icu_script_to_script(sc));
				hb_buffer_set_language(buf, hb_language_from_string("en-US", -1));

				hb_shape(font, buf, NULL, 0);

				unsigned int glyph_count;
				hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
				hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
				//TimeCheck("Getglyf start");

				for (unsigned int i = 0; i < glyph_count; i++) {
					auto& info = glyph_info[i];
					hb_codepoint_t glyphid = glyph_info[i].codepoint;
					hb_position_t x_offset = glyph_pos[i].x_offset;
					hb_position_t y_offset = glyph_pos[i].y_offset;
					hb_position_t x_advance = glyph_pos[i].x_advance;
					hb_position_t y_advance = glyph_pos[i].y_advance;
					/* draw_glyph(glyphid, cursor_x + x_offset, cursor_y + y_offset); */
					//std::cout << x_advance / 64.f << "\t" << y_advance / 64.f << std::endl;
					auto sg = ff->GetGlyph(glyphid);
					hb_position_t x = cursor_x + x_offset;
					hb_position_t y = cursor_y + y_offset;
					if (sg)
						sb->Push(sg, &(Matrix4x4::Translation({ x / scale.x ,y / scale.y  ,0 })));
					cursor_x += x_advance;
					cursor_y += y_advance;
				}
				//TimeCheck("Getglyf end");
				//TimeCheckSum();
				hb_buffer_destroy(buf);

				//UScriptRun scriptRun(tex, startRun, lengthRun);
				//while (scriptRun.next()) {
				//	int32_t     start = scriptRun.getScriptStart();
				//	int32_t     end = scriptRun.getScriptEnd();
				//	UScriptCode code = scriptRun.getScriptCode();

				//	printf("Script '%s' from %d to %d.\n", uscript_getName(code), start, end);
				//}
			}
		}
	}




	hb_font_destroy(font);
	return sb->Commit();
}