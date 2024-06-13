#include <iostream>
#include "win32impl.h"
#include "stdio.h"
#include <stdarg.h>
#include "reactive.h"
using namespace RLL;
class TT
{
public:
	TT(int a, float b, double c)
	{
	}
};
int main()
{
	R<TT> test(1, 0.5f, 0.8);
	R<int> tint(16);
	tint.listener() += ([](int o, int n) {
		printf("watch tint: %d -> %d\n", o, n);
		});

	tint = 8;
	tint = 128;
	printf("final: %d\n", *(tint.value));

	R<const char*> tcstr("asdf");
	tcstr.listener() += ([](const char* o, const char* n) {
		printf("watch tcstr: %s -> %s\n", o, n);
		});

	tcstr = "8asdawf";
	tcstr = "aaaaaaaaa";
	printf("final: %s\n", tcstr.operator->());

	std::cout <<
		"TEST ReactiveLayoutProject" << std::endl;

	printf("%#5x: %#5x\r\n", unsigned char(56), unsigned char(78));

	int c;
#ifdef _DEBUG
	LoadLibrary(L"C:\\Program Files\\Microsoft PIX\\2405.15.002-OneBranch_release\\WinPixGpuCapturer.dll");
#endif
	RLL::Initiate(INIT_DIRECTX_DEBUG_LAYER);



	IFrame* f = RLL::CreateFrame(nullptr, { 800,600 }, { 1920 / 2 - 400,1080 / 2 - 300 });
	f->Show();
	f->Run();

	return 0;
}