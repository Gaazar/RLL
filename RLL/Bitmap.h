#pragma once
#include "Sharp.h"

namespace Sharp
{
	namespace Graphics {
		class Bitmap
		{
			Byte* buffer = nullptr;
			Int width;
			Int height;
			Int pitch;
			Int bits;
		public:
			Bitmap();
			Bitmap(Sharp::Stream&);
			Int SaveTo(Sharp::Stream&);
			void CopyFrom(Byte* buffer, Int width, Int height, Int pitch = 0, Int bits = 24);
		};
	}
}
