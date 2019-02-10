#include "MemoryUtil.h"

bool memutil::WC2MB(LPWCH wcString, int wcCount, LPCH mbString, int mbMaxSize)
{
	int size = WideCharToMultiByte(CP_UTF8, 0, wcString, wcCount, nullptr, 0, nullptr, nullptr);
	if (size <= 0 || size > mbMaxSize)
		return false;
	WideCharToMultiByte(CP_UTF8, 0, wcString, wcCount, mbString, size, nullptr, nullptr);
	mbString[size] = 0;
	return true;
}

bool memutil::IsBadReadPtr(BYTE* Ptr)
{
	MEMORY_BASIC_INFORMATION MBI = {0};
	if (VirtualQuery(Ptr, &MBI, sizeof(MBI)))
	{
		DWORD dwMask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool bResult = !(MBI.Protect & dwMask);
		if (MBI.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			bResult = true;

		return bResult;
	}
	return true;
}

BYTE* memutil::ReadMultiLvlPtr(BYTE* BaseAddr, std::vector<DWORD> OffsetList)
{
	if (IsBadReadPtr(BaseAddr) || OffsetList.empty())
		return nullptr;

	BYTE* RetAddr = BaseAddr;
	size_t i, j = OffsetList.size() - 1;

	memcpy(&RetAddr, RetAddr, ptr_size);

	for (i = 0; i < OffsetList.size(); ++i)
	{
		RetAddr += OffsetList[i];

		if (i == j)
			break;

		if (IsBadReadPtr(RetAddr))
			return nullptr;

		memcpy(&RetAddr, RetAddr, ptr_size);
	}

	return (IsBadReadPtr(RetAddr) ? nullptr : RetAddr);
}
