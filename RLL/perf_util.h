#pragma once

#ifndef NO_TIMECHECKER
#include <stdint.h>

// return us
inline float GetTime();
inline void TimeCheck(char* name);
inline void TimeCheckSum();
#else
#define GetTime(...)
#define TimeCheck(...)
#define TimeCheckSum(...)
#endif