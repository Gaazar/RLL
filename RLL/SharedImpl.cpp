#pragma once
#include "Sharp.h"
#include <locale>
#include <codecvt>
#include <string>
#include <stdint.h>
#ifdef __GNUC__
#include <endian.h>
#endif // __GNUC__

static inline uint16_t byteswap_ushort(uint16_t number)
{
#if defined(_MSC_VER) && _MSC_VER > 1310
	return _byteswap_ushort(number);
#elif defined(__GNUC__)
	return __builtin_bswap16(number);
#else
	return (number >> 8) | (number << 8);
#endif
}

using namespace Sharp;

#pragma region StringImplement
static String empty;
String& String::EmptyString()
{
	return empty;
}
String::StringView::~StringView()
{
	if (EmptyString().strview != nullptr && str != EmptyString().strview->str && str != L"")
	{
		free(str);
	}
}
String::String()
{
	if (EmptyString().strview)
	{
		strview = EmptyString().strview;
	}
	else
	{
		strview = std::make_shared<StringView>();
	}
}
String::String(const wchar_t* chars)
{
	strview = std::make_shared<StringView>();
	strview->length = wcslen(chars);
	strview->str = (wchar_t*)Malloc(strview->length * 2 + 2);
	wcscpy(strview->str, chars);
}
String::String(const char* chars)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
	auto cs = convert.from_bytes(chars);
	String s(cs.c_str());
	strview = s.strview;
}
String::String(String& str)
{
	strview = str.strview;
}
String::~String()
{
	strview = nullptr;
}
int String::Length()
{
	return strview->length;
}
String& String::Append(String s)
{
	String ns = *this + s;
	strview = s.strview;
	return *this;
}
String& String::Prepend(String s)
{
	String ns = s + *this;
	strview = s.strview;
	return *this;
}
String& String::Insert(int index, String s)
{
	throw Exception("No implements method.");
	return *this;
}
String& String::Replace(String s)
{
	throw Exception("No implements method.");
	return *this;
}
bool String::Contains(String s)
{
	throw Exception("No implements method.");
	return false;
}
bool String::StartsWith(String s)
{
	if (s.Length() > Length()) return false;
	for (auto i = 0; i < s.Length(); i++)
	{
		if (s[i] != operator[](i))
			return false;
	}
	return true;
}
bool String::EndsWith(String s)
{
	if (s.Length() > Length()) return false;
	auto tl = Length() - 1;
	auto sl = s.Length() - 1;
	for (auto i = 0; i < s.Length(); i++)
	{
		if (s[sl - i] != operator[](tl - i))
			return false;
	}
	return true;
}
String String::Left(int count)
{
	wchar_t* ss = (wchar_t*)Malloc((count + 1) * 2);
	memcpy(ss, strview->str, count * sizeof(wchar_t));
	ss[count] = 0;
	return String(ss, count);
}
String String::Right(int count)
{
	wchar_t* ss = (wchar_t*)Malloc((count + 1) * 2);
	wcscpy(ss, strview->str + Length() - count);
	return String(ss, count);
}
std::vector<String>& String::Split(String spliter)
{
	throw Exception("No implements method.");
	std::vector<String> result;
	return result;
}
wchar_t& String::operator[] (int index)
{
	if (index < 0 || index >= strview->length)
	{
		throw Exception("index of out range");
	}
	return strview->str[index];
}
String::String(wchar_t* s, int len)
{
	strview = std::make_shared<StringView>();
	strview->length = len;
	strview->str = s;
}
String String::operator+ (String& s)
{
	auto rstr = (wchar_t*)Malloc((Length() + s.Length() + 1) * 2);
	wcscpy(rstr, strview->str);
	wcscat(rstr, s.strview->str);
	return String(rstr, Length() + s.Length());
}
#pragma endregion
#pragma region ExceptionImplement
Exception::Exception()
{
	reason = "Unknown exception";
};
Exception::Exception(String reason)
{
	this->reason = reason;
};
String Exception::ToString()
{
	return reason;
};
#pragma endregion

//     ����ת��������С�����½���     //

std::string utf16_to_utf8(const std::u16string& u16str);
// ��UTF16 LE������ַ�������
std::string utf16le_to_utf8(const std::u16string& u16str);
// ��UTF16BE�����ַ�������
std::string utf16be_to_utf8(const std::u16string& u16str);
// ��ȡת��ΪUTF-16 LE������ַ���
std::u16string utf8_to_utf16le(const std::string& u8str, bool addbom = false, bool* ok = NULL);
// ��ȡת��ΪUTF-16 BE���ַ���
std::u16string utf8_to_utf16be(const std::string& u8str, bool addbom = false, bool* ok = NULL);
// ��UTF16�����ַ�����������Ҫ��BOM���
std::string utf16_to_utf8(const std::u16string& u16str)
{
	if (u16str.empty()) { return std::string(); }
	//Byte Order Mark
	char16_t bom = u16str[0];
	switch (bom) {
	case 0xFEFF:    //Little Endian
		return utf16le_to_utf8(u16str);
		break;
	case 0xFFFE:    //Big Endian
		return utf16be_to_utf8(u16str);
		break;
	default:
		return std::string();
	}
}
// ��UTF16 LE������ַ�������
std::string utf16le_to_utf8(const std::u16string& u16str)
{
	if (u16str.empty()) { return std::string(); }
	const char16_t* p = u16str.data();
	std::u16string::size_type len = u16str.length();
	if (p[0] == 0xFEFF) {
		p += 1; //����bom��ǣ�����
		len -= 1;
	}

	// ��ʼת��
	std::string u8str;
	u8str.reserve(len * 3);

	char16_t u16char;
	for (std::u16string::size_type i = 0; i < len; ++i) {
		// �����������С������(���������)
		u16char = p[i];

		// 1�ֽڱ�ʾ����
		if (u16char < 0x0080) {
			// u16char <= 0x007f
			// U- 0000 0000 ~ 0000 07ff : 0xxx xxxx
			u8str.push_back((char)(u16char & 0x00FF));  // ȡ��8bit
			continue;
		}
		// 2 �ֽ��ܱ�ʾ����
		if (u16char >= 0x0080 && u16char <= 0x07FF) {
			// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
			u8str.push_back((char)(((u16char >> 6) & 0x1F) | 0xC0));
			u8str.push_back((char)((u16char & 0x3F) | 0x80));
			continue;
		}
		// ������Բ���(4�ֽڱ�ʾ)
		if (u16char >= 0xD800 && u16char <= 0xDBFF) {
			// * U-00010000 - U-001FFFFF: 1111 0xxx 10xxxxxx 10xxxxxx 10xxxxxx
			uint32_t highSur = u16char;
			uint32_t lowSur = p[++i];
			// �Ӵ�����Ե�UNICODE�����ת��
			// 1���Ӹߴ������ȥ0xD800����ȡ��Ч10bit
			// 2���ӵʹ������ȥ0xDC00����ȡ��Ч10bit
			// 3������0x10000����ȡUNICODE�����ֵ
			uint32_t codePoint = highSur - 0xD800;
			codePoint <<= 10;
			codePoint |= lowSur - 0xDC00;
			codePoint += 0x10000;
			// תΪ4�ֽ�UTF8�����ʾ
			u8str.push_back((char)((codePoint >> 18) | 0xF0));
			u8str.push_back((char)(((codePoint >> 12) & 0x3F) | 0x80));
			u8str.push_back((char)(((codePoint >> 06) & 0x3F) | 0x80));
			u8str.push_back((char)((codePoint & 0x3F) | 0x80));
			continue;
		}
		// 3 �ֽڱ�ʾ����
		{
			// * U-0000E000 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
			u8str.push_back((char)(((u16char >> 12) & 0x0F) | 0xE0));
			u8str.push_back((char)(((u16char >> 6) & 0x3F) | 0x80));
			u8str.push_back((char)((u16char & 0x3F) | 0x80));
			continue;
		}
	}

	return u8str;
}
// ��UTF16BE�����ַ�������
std::string utf16be_to_utf8(const std::u16string& u16str)
{
	if (u16str.empty()) { return std::string(); }
	const char16_t* p = u16str.data();
	std::u16string::size_type len = u16str.length();
	if (p[0] == 0xFEFF) {
		p += 1; //����bom��ǣ�����
		len -= 1;
	}


	// ��ʼת��
	std::string u8str;
	u8str.reserve(len * 2);
	char16_t u16char;   //u16le ���ֽڴ��λ�����ֽڴ��λ
	for (std::u16string::size_type i = 0; i < len; ++i) {
		// �����������С������(���������)
		u16char = p[i];
		// �������תΪС����
		u16char = byteswap_ushort(u16char);

		// 1�ֽڱ�ʾ����
		if (u16char < 0x0080) {
			// u16char <= 0x007f
			// U- 0000 0000 ~ 0000 07ff : 0xxx xxxx
			u8str.push_back((char)(u16char & 0x00FF));
			continue;
		}
		// 2 �ֽ��ܱ�ʾ����
		if (u16char >= 0x0080 && u16char <= 0x07FF) {
			// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
			u8str.push_back((char)(((u16char >> 6) & 0x1F) | 0xC0));
			u8str.push_back((char)((u16char & 0x3F) | 0x80));
			continue;
		}
		// ������Բ���(4�ֽڱ�ʾ)
		if (u16char >= 0xD800 && u16char <= 0xDBFF) {
			// * U-00010000 - U-001FFFFF: 1111 0xxx 10xxxxxx 10xxxxxx 10xxxxxx
			uint32_t highSur = u16char;
			uint32_t lowSur = byteswap_ushort(p[++i]);
			// �Ӵ�����Ե�UNICODE�����ת��
			// 1���Ӹߴ������ȥ0xD800����ȡ��Ч10bit
			// 2���ӵʹ������ȥ0xDC00����ȡ��Ч10bit
			// 3������0x10000����ȡUNICODE�����ֵ
			uint32_t codePoint = highSur - 0xD800;
			codePoint <<= 10;
			codePoint |= lowSur - 0xDC00;
			codePoint += 0x10000;
			// תΪ4�ֽ�UTF8�����ʾ
			u8str.push_back((char)((codePoint >> 18) | 0xF0));
			u8str.push_back((char)(((codePoint >> 12) & 0x3F) | 0x80));
			u8str.push_back((char)(((codePoint >> 06) & 0x3F) | 0x80));
			u8str.push_back((char)((codePoint & 0x3F) | 0x80));
			continue;
		}
		// 3 �ֽڱ�ʾ����
		{
			// * U-0000E000 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
			u8str.push_back((char)(((u16char >> 12) & 0x0F) | 0xE0));
			u8str.push_back((char)(((u16char >> 6) & 0x3F) | 0x80));
			u8str.push_back((char)((u16char & 0x3F) | 0x80));
			continue;
		}
	}
	return u8str;
}
// ��ȡת��ΪUTF-16 LE������ַ���
std::u16string utf8_to_utf16le(const std::string& u8str, bool addbom, bool* ok)
{
	std::u16string u16str;
	u16str.reserve(u8str.size());
	if (addbom) {
		u16str.push_back(0xFEFF);   //bom (�ֽڱ�ʾΪ FF FE)
	}
	std::string::size_type len = u8str.length();

	const unsigned char* p = (unsigned char*)(u8str.data());
	// �ж��Ƿ����BOM(�жϳ���С��3�ֽڵ����)
	if (len > 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
		p += 3;
		len -= 3;
	}

	bool is_ok = true;
	// ��ʼת��
	for (std::string::size_type i = 0; i < len; ++i) {
		uint32_t ch = p[i]; // ȡ��UTF8�������ֽ�
		if ((ch & 0x80) == 0) {
			// ���λΪ0��ֻ��1�ֽڱ�ʾUNICODE�����
			u16str.push_back((char16_t)ch);
			continue;
		}
		switch (ch & 0xF0)
		{
		case 0xF0: // 4 �ֽ��ַ�, 0x10000 �� 0x10FFFF
		{
			uint32_t c2 = p[++i];
			uint32_t c3 = p[++i];
			uint32_t c4 = p[++i];
			// ����UNICODE�����ֵ(��һ���ֽ�ȡ��3bit������ȡ6bit)
			uint32_t codePoint = ((ch & 0x07U) << 18) | ((c2 & 0x3FU) << 12) | ((c3 & 0x3FU) << 6) | (c4 & 0x3FU);
			if (codePoint >= 0x10000)
			{
				// ��UTF-16�� U+10000 �� U+10FFFF ������16bit��Ԫ��ʾ, �������.
				// 1����������ȥ0x10000(�õ�����Ϊ20bit��ֵ)
				// 2��high ������ �ǽ���20bit�еĸ�10bit����0xD800(110110 00 00000000)
				// 3��low  ������ �ǽ���20bit�еĵ�10bit����0xDC00(110111 00 00000000)
				codePoint -= 0x10000;
				u16str.push_back((char16_t)((codePoint >> 10) | 0xD800U));
				u16str.push_back((char16_t)((codePoint & 0x03FFU) | 0xDC00U));
			}
			else
			{
				// ��UTF-16�� U+0000 �� U+D7FF �Լ� U+E000 �� U+FFFF ��Unicode�����ֵ��ͬ.
				// U+D800 �� U+DFFF ����Ч�ַ�, Ϊ�˼���������������������(������򲻱���)
				u16str.push_back((char16_t)codePoint);
			}
		}
		break;
		case 0xE0: // 3 �ֽ��ַ�, 0x800 �� 0xFFFF
		{
			uint32_t c2 = p[++i];
			uint32_t c3 = p[++i];
			// ����UNICODE�����ֵ(��һ���ֽ�ȡ��4bit������ȡ6bit)
			uint32_t codePoint = ((ch & 0x0FU) << 12) | ((c2 & 0x3FU) << 6) | (c3 & 0x3FU);
			u16str.push_back((char16_t)codePoint);
		}
		break;
		case 0xD0: // 2 �ֽ��ַ�, 0x80 �� 0x7FF
		case 0xC0:
		{
			uint32_t c2 = p[++i];
			// ����UNICODE�����ֵ(��һ���ֽ�ȡ��5bit������ȡ6bit)
			uint32_t codePoint = ((ch & 0x1FU) << 12) | ((c2 & 0x3FU) << 6);
			u16str.push_back((char16_t)codePoint);
		}
		break;
		default:    // ���ֽڲ���(ǰ���Ѿ��������Բ�Ӧ�ý���)
			is_ok = false;
			break;
		}
	}
	if (ok != NULL) { *ok = is_ok; }

	return u16str;
}
// ��ȡת��ΪUTF-16 BE���ַ���
std::u16string utf8_to_utf16be(const std::string& u8str, bool addbom, bool* ok)
{
	// �Ȼ�ȡutf16le�����ַ���
	std::u16string u16str = utf8_to_utf16le(u8str, addbom, ok);
	// ��С����ת��Ϊ�����
	for (size_t i = 0; i < u16str.size(); ++i) {
		u16str[i] = byteswap_ushort(u16str[i]);
	}
	return u16str;
}

bool BasicWriter::Write(String& value)
{
	std::u16string ws = (char16_t*)value.CString();
	std::string s = utf16le_to_utf8(ws);
	uInt len = s.length();
	stream->Write((Byte*)&len, sizeof uint32_t);
	stream->Write((Byte*)s.c_str(), len);
	return true;
}

bool BasicReader::Read(String& value)
{
	uInt len;
	stream->Read((Byte*)&len, sizeof len);
	Byte* cs = (Byte*)Malloc(len + 1);
	stream->Read(cs, len);
	cs[len] = 0;
	value = (wchar_t*)utf8_to_utf16le(cs).c_str();
	free(cs);
	return true;
}