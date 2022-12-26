#include "Bitmap.h"
#include <iostream>

using namespace Sharp;
using namespace Sharp::Graphics;
#pragma pack(1)
struct BMPFileHeader {
	char type[2] = { 'B','M' };       //λͼ�ļ������ͣ�����ΪBM�����������Ͳ��ԣ�������ʾ����
	unsigned int size;                 //λͼ�ļ��Ĵ�С�����ֽ�Ϊ��λ
	short rd1 = 0;         // λͼ�ļ������֣�����Ϊ0
	short rd2 = 0;          // λͼ�ļ������֣�����Ϊ0
	unsigned int offset = 54;               // λͼ���ݵ���ʼλ�ã��������λͼ
};

struct BMPInfoHeader
{
	uInt   biSize = 40;             //ָ���˽ṹ��ĳ��ȣ�Ϊ40  
	Int    biWidth;            //λͼ��  
	Int    biHeight;           //λͼ��  
	uShort    biPlanes = 1;           //ƽ������Ϊ1  
	uShort    biBitCount = 24;         //������ɫλ����������1��2��4��8��16��24���µĿ�����32  
	uInt   biCompression = 0;      //ѹ����ʽ��������0��1��2������0��ʾ��ѹ��  
	uInt   biSizeImage;        //ʵ��λͼ����ռ�õ��ֽ���  
	uInt    biXPelsPerMeter = 2834;    //X����ֱ���  
	uInt    biYPelsPerMeter = 2834;    //Y����ֱ���  
	uInt   biClrUsed = 0;          //ʹ�õ���ɫ�������Ϊ0�����ʾĬ��ֵ(2^��ɫλ��)  
	uInt   biClrImportant = 0;     //��Ҫ��ɫ�������Ϊ0�����ʾ������ɫ������Ҫ��  
};

Bitmap::Bitmap() {

}
Bitmap::Bitmap(Sharp::Stream&) {

}
Int Bitmap::SaveTo(Sharp::Stream& s) {
	BMPFileHeader fh;
	BMPInfoHeader ih;
	Int p;
	if (pitch <= 0)
	{
		p = width;
	}
	else
	{
		p = pitch;
	}
	ih.biBitCount = bits;
	ih.biWidth = width;
	ih.biHeight = height;
	ih.biSizeImage = p * height * bits / 8;
	fh.size = ih.biSizeImage + 54;
	BasicWriter bw(s);
	bw.Write(fh);
	bw.Write(ih);
	int rowlen = width * bits / 8;
	for (int h = height - 1; h >= 0; h--)
	{
		s.Write((char*)buffer + p * h, p);

	}
	return fh.size;

}

void Bitmap::CopyFrom(Byte* buffer, Int width, Int height, Int pitch, Int bits)
{
	this->pitch = pitch;
	this->width = width;
	this->height = height;
	this->bits = bits;

	if (this->buffer)
	{
		free(this->buffer);
	}
	this->buffer = (Byte*)malloc(pitch * height );
	//for (int r = 0; r < height; r++)
	//{
	//	for (int p = 0; p < pitch; p++)
	//	{
	//		this->buffer[r * height * 3 + p] = buffer[r * height + p];
	//		this->buffer[r * height * 3 + p + 1] = buffer[r * height + p];
	//		this->buffer[r * height * 3 + p + 2] = buffer[r * height + p];

	//	}
	//}
	memcpy(this->buffer, buffer, pitch * height);
}