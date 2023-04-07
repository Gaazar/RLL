#include "win32paint_d12.h"
#include <algorithm>
#include "curve_tools.h"

#define NO_TIMECHECKER
#include "perf_util.h"
using namespace RLL;
using namespace std;
using namespace Math3D;


void D3D12Brush::Dispose()
{
	if (brush)
		brush->pad = 1;
	//device->flag |= FLAG_D12PD_BRUSH_DIRTY;
	delete this;
}

