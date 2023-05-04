#pragma once
#include "Interfaces.h"
#include "Painter.h"
#include <vector>
#include "unicode/utypes.h"
#include "unicode/uscript.h"

namespace RLL
{
	void TextLayoutInit();
	class IFontFace;
	class IFontFactory : public IBase
	{
	public:
		virtual IFontFace* LoadFromFile(char*) = 0;
		virtual void Dispose() = 0;
	};

	class IFontFace : public IBase
	{
	public:
		virtual void SetLevel() { NOIMPL; };
		virtual uint32_t GetCodepoint(char32_t unicode) = 0;
		virtual IGeometry* GetPlainGlyph(char32_t unicode, RLL::GlyphMetrics* metrics = nullptr) = 0;
		virtual RLL::IGeometry* GetPlainGlyph(uint32_t codepoint, RLL::GlyphMetrics* metrics = nullptr) = 0;

		virtual ISVG* GetGlyph(char32_t unicode, RLL::GlyphMetrics* metrics = nullptr) = 0;
		virtual RLL::ISVG* GetGlyph(uint32_t codepoint, RLL::GlyphMetrics* metrics = nullptr) = 0;

		virtual void Dispose() { NOIMPL; };
	};
	class IFontStack :public IFontFace
	{
	private:
		std::vector<IFontFace*> fonts;
	public:
		IFontStack() {};
		virtual uint32_t GetCodepoint(char32_t unicode) { assert(0);/*no avaliable.*/ return 0; }

		virtual IGeometry* GetPlainGlyph(char32_t unicode, RLL::GlyphMetrics* metrics = nullptr);
		virtual RLL::IGeometry* GetPlainGlyph(uint32_t codepoint, RLL::GlyphMetrics* metrics = nullptr) { assert(0);/*no avaliable.*/ return nullptr; }

		virtual ISVG* GetGlyph(char32_t unicode, RLL::GlyphMetrics* metrics = nullptr);
		virtual RLL::ISVG* GetGlyph(uint32_t codepoint, RLL::GlyphMetrics* metrics = nullptr) { assert(0);/*no avaliable.*/ return nullptr; }
		virtual void Push(RLL::IFontFace* face);
		virtual RLL::IFontFace* GetFace(char32_t unicode);
		virtual void Dispose() { NOIMPL; };

	};

	IFontFactory* CreateFontFactory(IPaintDevice*);

	class ITextLayout :public IBase
	{
	public:
		virtual void AppendText(char* text) = 0;
		virtual void InsertText(char* text, int pos) = 0;
		virtual RLL::ISVG* Commit() = 0;
		void Dispose() {};

	};

	class TextLayout :public IBase
	{
	private:
		template<typename T>
		struct Range
		{
			struct Pair
			{
				int32_t begin;
				int32_t end;
				T value;
			};
			std::vector<Pair> values;

			void Set(int32_t begin, int32_t end, T value)
			{
				Pair p = { begin,end,value };
				for (auto i = values.begin(); i != values.end(); ++i)
				{
					if (i->end >= begin)
					{
						i->end = begin;
						auto ii = i + 1;
						if (ii != values.end())
						{
							ii->begin = end;
						}
						values.insert(ii, p);
						break;
					}
				}
				values.push_back(p);
			}

			//std::vector<std::pair<INDEX, INDEX>> Intersect(INDEX begin, INDEX end)
			//{
			//	std::vector<std::pair<INDEX, INDEX>> res;
			//	bool ena = false;
			//	for (auto& i : values)
			//	{
			//		if (end >= i.begin && end < i.end)
			//		{
			//			res.push_back({ i.begin,end });
			//			ena = false;
			//			break;
			//		}
			//		if (ena)
			//			res.push_back({ i.begin,i.end });
			//		if (begin >= i.begin && begin < i.end)
			//		{
			//			res.push_back({ begin,i.end });
			//			ena = true;
			//		}
			//	}
			//	return res;
			//}

			T operator[](int32_t index)
			{
				for (auto i = values.begin(); i != values.end(); ++i)
				{
					if (index >= i->begin && index < i->end)
					{
						return i->value;
					}
				}
				if (values.size() > 0)
					return values[0].value;
				assert(0);
			}

		};
		struct LA
		{
			LINE_ALIGN x = LINE_ALIGN_START;
			LINE_ALIGN y = LINE_ALIGN_BASELINE;
		} lineAlign;
		struct BreakPart
		{
			Math3D::float2 size;
			Math3D::float2 pxpem;
			bool rtl = false;
			UScriptCode script;
			std::vector<Math3D::float2> glyfOffset;
			std::vector<Math3D::float2> glyfScale;
			std::vector<RLL::ISVG*> glyfs;
			//int32_t charIndex;
			//int32_t charLength;
		};
		struct FontSplit
		{
			UChar* ucs;
			int32_t len;
			IFontFace* face;
			UScriptCode script;
		};
		void* text;
		int32_t textLen;
		RLL::Size rectSize;
		Range<RLL::IFontFace*> ffaces;
		Range<float> fsizes;
		std::vector<FontSplit> split;
		std::vector<BreakPart> parts;

	public:
		void Metrics();
		void Break();
		void Place(RLL::ISVGBuilder*);
	public:
		TextLayout(wchar_t* text, RLL::Size size, RLL::IFontFace* fface);
		void SetParagraphAlign(PARA_ALIGN axis, PARA_ALIGN biAxis);
		void SetLineAlign(LINE_ALIGN axis, LINE_ALIGN biAxis);
		void SetFontFace(RLL::IFontFace* ff, int32_t begin, int32_t len);
		RLL::ISVG* Commit(RLL::IPaintDevice* dvc);
		void Dispose();
	};
}