#pragma once
#include <vector>
#include <atomic>
#include <string>

#pragma disable 4305

namespace Gz
{
	template <typename _T>
	using Vector = std::vector<_T>;

	template <typename _T>
	using Atomic = std::atomic<_T>;

	using String = std::string;

}