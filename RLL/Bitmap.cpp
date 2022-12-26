#include "Bitmap.h"
#include <iostream>

using namespace Sharp;
using namespace Sharp::Graphics;
#pragma pack(1)
struct BMPFileHeader {
	char type[2] = { 'B','M' };       //位图文件的类型，必须为BM，我这里类型不对，所以显示有误。
	unsigned int size;                 //位图文件的大小，以字节为单位
	short rd1 = 0;         // 位图文件保留字，必须为0
	short rd2 = 0;          // 位图文件保留字，必须为0
	unsigned int offset = 54;               // 位图数据的起始位置，以相对于位图
};

struct BMPInfoHeader
{
	uInt   biSize = 40;             //指定此结构体的长度，为40  
	Int    biWidth;            //位图宽  
	Int    biHeight;           //位图高  
	uShort    biPlanes = 1;           //平面数，为1  
	uShort    biBitCount = 24;         //采用颜色位数，可以是1，2，4，8，16，24，新的可以是32  
	uInt   biCompression = 0;      //压缩方式，可以是0，1，2，其中0表示不压缩  
	uInt   biSizeImage;        //实际位图数据占用的字节数  
	uInt    biXPelsPerMeter = 2834;    //X方向分辨率  
	uInt    biYPelsPerMeter = 2834;    //Y方向分辨率  
	uInt   biClrUsed = 0;          //使用的颜色数，如果为0，则表示默认值(2^颜色位数)  
	uInt   biClrImportant = 0;     //重要颜色数，如果为0，则表示所有颜色都是重要的  
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