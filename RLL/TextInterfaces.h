#pragma once
#include "Interfaces.h"
#include "Painter.h"
namespace RLL
{
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
		virtual IGeometry* GetPlainGlyph(char32_t unicode) = 0;
		virtual RLL::IGeometry* GetPlainGlyph(uint32_t codepoint) = 0;

		virtual ISVG* GetGlyph(char32_t unicode) = 0;
		virtual RLL::ISVG* GetGlyph(uint32_t codepoint) = 0;
		virtual void Dispose() = 0;
	};
	class IFontStack :public IFontFace
	{

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
}