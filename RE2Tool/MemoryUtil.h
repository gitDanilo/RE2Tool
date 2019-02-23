#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <vector>

#define AOB2DWORD(A, B) (((A)[3 + (B)] << 24) | ((A)[2 + (B)] << 16) | ((A)[1 + (B)] << 8) | ((A)[(B)]))

#define PROCESS_NAME "re2.exe"
#define MAX_BUF_SIZE 32
#define MAX_ENTITY_COUNT 32

typedef unsigned long long QWORD;

static constexpr QWORD qwOneHourInMS = 3600000000;
static constexpr QWORD qwOneMinInMS = 60000000;
static constexpr QWORD qwOneSecInMS = 1000000;

typedef struct _AOB_PATTERN
{
	DWORD dwModuleID;
	DWORD dwOffset;
	DWORD dwExtra;
	BYTE* aobPattern;
	DWORD dwPatternSize;
	BYTE* Address;
	DWORD dwAddrOffset;
} AOB_PATTERN, *PAOB_PATTERN;

typedef struct _RE_DATA
{
	BYTE bIsInControl;
	INT iPlayerMaxHP;
	INT iPlayerHP;
	QWORD qwActiveTime;
	QWORD qwCutsceneTime;
	QWORD qwPausedTime;
	INT iLastDmg;
	INT iEntityCount;
	INT EntityMaxHPList[MAX_ENTITY_COUNT];
	INT EntityHPList[MAX_ENTITY_COUNT];
	void GetFormatedTime(char* buf);
} RE_DATA, *PRE_DATA;

namespace memutil
{
	bool WC2MB(LPWCH wcString, int wcCount, LPCH mbString, int mbMaxSize);
	bool IsBadReadPtr(BYTE* Ptr);
	BYTE* ReadMultiLvlPtr(BYTE* BaseAddr, std::vector<DWORD> OffsetList);
	bool CompareByteArray(const BYTE* ByteArray1, const BYTE* ByteArray2, DWORD dwLength);
	BYTE* AOBScan(const BYTE* AOB, DWORD dwImageSize, const BYTE* TargetAOB, DWORD dwLength);
	bool GetModuleInfo(const char* sModuleName, BYTE* &ModuleBaseAddr, DWORD &dwModuleSize);
	bool PatchExecMemory(BYTE* MemAddr, const BYTE* aobPatch, DWORD dwPatchSize);
	LPVOID AllocExecMem(const BYTE* aobFunction, DWORD dwFunctionSize);
}
