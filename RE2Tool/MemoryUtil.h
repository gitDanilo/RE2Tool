#pragma once

#include <Windows.h>
#include <vector>

constexpr DWORD ptr_size = sizeof(BYTE*);

typedef unsigned long long QWORD;

namespace memutil
{
	bool WC2MB(LPWCH wcString, int wcCount, LPCH mbString, int mbMaxSize);
	bool IsBadReadPtr(BYTE* Ptr);
	BYTE* ReadMultiLvlPtr(BYTE* BaseAddr, std::vector<DWORD> OffsetList);
}
