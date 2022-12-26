#pragma once
#include "Metrics.h"

enum TEXT_RENDER_FLAG {
	NONE = 0,
	USE_HINT = 1 << 1,
	DYNAMIC_HINT = 1 << 2,
};
inline TEXT_RENDER_FLAG operator|(TEXT_RENDER_FLAG a, TEXT_RENDER_FLAG b)
{
	return static_cast<TEXT_RENDER_FLAG>(static_cast<int>(a) | static_cast<int>(b));
}