#pragma once

#ifndef NO_TIMECHECKER
#include <stdint.h>

// return us
float GetTime();
void TimeCheck(char* name);
void TimeCheckSum();
#else
#define GetTime(...)
#define TimeCheck(...)
#define TimeCheckSum(...)
#endif