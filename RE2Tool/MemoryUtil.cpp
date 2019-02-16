#include "MemoryUtil.h"

void _RE_DATA::GetFormatedTime(char* buf)
{
	QWORD qwTime = this->qwActiveTime - this->qwCutsceneTime - this->qwPausedTime;
	QWORD qwH = 0, qwM = 0, qwS = 0;

	qwH = qwTime / qwOneHourInMS;
	if (qwTime % qwOneHourInMS)
	{
		qwTime -= qwH * qwOneHourInMS;
		qwM = qwTime / qwOneMinInMS;
		if (qwTime % qwOneMinInMS)
		{
			qwTime -= qwM * qwOneMinInMS;
			qwS = qwTime / qwOneSecInMS;
		}
	}

	sprintf_s(buf, MAX_BUF_SIZE, "%02llu:%02llu:%02llu", qwH, qwM, qwS);
}

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

	RetAddr = reinterpret_cast<BYTE*>(*(reinterpret_cast<uintptr_t*>(RetAddr)));

	for (i = 0; i < OffsetList.size(); ++i)
	{
		RetAddr += OffsetList[i];

		if (i == j)
			break;

		if (IsBadReadPtr(RetAddr))
			return nullptr;

		RetAddr = reinterpret_cast<BYTE*>(*(reinterpret_cast<uintptr_t*>(RetAddr)));
	}

	return (IsBadReadPtr(RetAddr) ? nullptr : RetAddr);
}

bool memutil::CompareByteArray(const BYTE* ByteArray1, const BYTE* ByteArray2, DWORD dwLength)
{
	for (DWORD i = 0; i < dwLength; ++i)
	{
		if (ByteArray2[i] != 0xCC && ByteArray1[i] != ByteArray2[i])
		{
			return false;
		}
	}
	return true;
}

BYTE* memutil::AOBScan(const BYTE* AOB, DWORD dwImageSize, const BYTE* TargetAOB, DWORD dwLength)
{
	BYTE* Addr = nullptr;
	DWORD dwFinalAddress = dwImageSize - dwLength;
	for (DWORD i = 0; i < dwFinalAddress; ++i)
	{
		if (CompareByteArray((AOB + i), TargetAOB, dwLength))
		{
			Addr = const_cast<BYTE*>(AOB + i);
			break;
		}
	}
	return Addr;
}

bool memutil::GetModuleInfo(const char* sModuleName, BYTE* &ModuleBaseAddr, DWORD &dwModuleSize)
{
	MODULEENTRY32 ME32;
	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());

	ModuleBaseAddr = 0;
	dwModuleSize = 0;
	ME32.dwSize = sizeof(MODULEENTRY32);

	if (!Module32First(hModuleSnap, &ME32))
	{
		CloseHandle(hModuleSnap);
		return false;
	}

	do
	{
		if (!strcmp(ME32.szModule, sModuleName))
		{
			ModuleBaseAddr = ME32.modBaseAddr;
			dwModuleSize = ME32.modBaseSize;
			break;
		}
	} while (Module32Next(hModuleSnap, &ME32));

	CloseHandle(hModuleSnap);
	return (ModuleBaseAddr != 0);
}
