#pragma once
#include "windows.h"
#include "Interfaces.h"
#include "PathGeometry.h"
#define INIT_DIRECTX_DEBUG_LAYER (0x1)
#define INIT_DEVELOP (0x2)

namespace RLL
{
	void Initiate(int flag = 0);
	int GetFlags();
	wchar_t* GetLocale();
	Math3D::Vector2 GetScale();
	IFrame* CreateFrame(IFrame* parent, Math3D::Vector2 size, Math3D::Vector2 pos);

}