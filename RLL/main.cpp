#include <iostream>
#include "win32impl.h"

using namespace RLL;
int main()
{
	std::cout <<
		"asdf" << std::endl;
	int c;
#ifdef _DEBUG
	LoadLibrary(L"C:\\Program Files\\Microsoft PIX\\2201.24\\WinPixGpuCapturer.dll");
#endif
	RLL::Initiate(INIT_DIRECTX_DEBUG_LAYER);
	IFrame* f = RLL::CreateFrame(nullptr, { 800,600 }, { 1920 / 2 - 400,1080 / 2 - 300 });
	f->Show();
	f->Run();

	return 0;
}