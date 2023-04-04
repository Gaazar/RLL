#include "perf_util.h"
#include <windows.h>
#include <vector>
#include <iostream>

struct TimeCheckPoint
{
	char* name;
	uint64_t time_ms;
};
std::vector<TimeCheckPoint> tckps;
float GetTime()
{
	LARGE_INTEGER m_liPerfFreq = { 0 };
	QueryPerformanceFrequency(&m_liPerfFreq);
	LARGE_INTEGER m_liPerfStart = { 0 };
	QueryPerformanceCounter(&m_liPerfStart);
	return (float)m_liPerfStart.QuadPart / m_liPerfFreq.QuadPart * 1000.0f;

}
uint64_t time_freq = 0;
uint64_t _GetTime()
{
	LARGE_INTEGER m_liPerfFreq = { 0 };
	QueryPerformanceFrequency(&m_liPerfFreq);
	LARGE_INTEGER m_liPerfStart = { 0 };
	QueryPerformanceCounter(&m_liPerfStart);
	time_freq = m_liPerfFreq.QuadPart;
	return m_liPerfStart.QuadPart;
}
uint64_t time_begin = 0;
void TimeCheck(char* name)
{
	if (tckps.size() == 0)
	{
		time_begin = _GetTime();
	}
	tckps.push_back({ name,_GetTime() - time_begin });
}
void TimeCheckSum()
{
	for (auto& i : tckps)
	{
		std::cout << "TimeChecker:\t" << i.name << "\tat: " << (float)i.time_ms/time_freq * 1000.f << "ms" << std::endl;
	}
	tckps.clear();
}