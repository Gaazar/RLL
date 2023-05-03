#include "TextInterfaces.h"

using namespace RLL;
using namespace Math3D;

ISVG* IFontStack::GetGlyph(char32_t unicode, RLL::GlyphMetrics* metrics )
{
	for (auto i = fonts.rbegin(); i != fonts.rend(); ++i)
	{
		auto code = (*i)->GetCodepoint(unicode);
		if (code != 0)
		{
			return (*i)->GetGlyph(code, metrics);
		}
	}
	return nullptr;
}
IGeometry* IFontStack::GetPlainGlyph(char32_t unicode, RLL::GlyphMetrics* metrics )
{
	for (auto i = fonts.rbegin(); i != fonts.rend(); ++i)
	{
		auto code = (*i)->GetCodepoint(unicode);
		if (code != 0)
		{
			return (*i)->GetPlainGlyph(code, metrics);
		}
	}
	return nullptr;

}

void IFontStack::Push(RLL::IFontFace* face)
{
	if (!face)
		return;
	for (auto i = fonts.begin(); i != fonts.end(); i++)
	{
		if (*i == face)
		{
			fonts.erase(i);
			break;
		}
	}
	fonts.push_back(face);

}
RLL::IFontFace* IFontStack::GetFace(char32_t unicode)
{
	if (fonts.size() == 0) return nullptr;
	for (auto i = fonts.rbegin(); i != fonts.rend(); ++i)
	{
		auto code = (*i)->GetCodepoint(unicode);
		if (code != 0)
		{
			return *i;
		}
	}
	return fonts[0];

}
