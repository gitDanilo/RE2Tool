#include <iostream>
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "REFramework.h"

// Global variables
HMODULE g_dinput = 0;

// Method declaration
DWORD WINAPI startupThread();
void failedMsg();

extern "C"
{
	__declspec(dllexport) HRESULT WINAPI DirectInput8Create_(HINSTANCE hinst, DWORD dwVersion, const IID& riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
	{
	#pragma comment(linker, "/EXPORT:DirectInput8Create=DirectInput8Create_")
		return ((decltype(DirectInput8Create)*)GetProcAddress(g_dinput, "DirectInput8Create"))(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	}
}

void failedMsg()
{
	MessageBox(0, "Unable to load the original dinput8.dll.", "RE2Tool", 0);
	ExitProcess(0);
}

DWORD WINAPI startupThread()
{
	// init
	wchar_t buffer[MAX_PATH] {0};
	if (GetSystemDirectoryW(buffer, MAX_PATH) == 0)
		failedMsg();

	g_dinput = LoadLibraryW((std::wstring {buffer} + L"\\dinput8.dll").c_str());
	if (g_dinput == NULL)
		failedMsg();

	AllocConsole();

	SetConsoleTitle("RE2Tool");

	FILE* pFile;
	freopen_s(&pFile, "CONIN$", "r", stdin);
	freopen_s(&pFile, "CONOUT$", "w", stdout);
	freopen_s(&pFile, "CONOUT$", "w", stderr);

	std::cout << "RE2Tool DLL Injected." << std::endl;

	MessageBox(0, "Press OK to continue.", "RE2Tool", 0);

	gREFramework = std::make_unique<REFramework>();

	ExitThread(0);
}

BOOL APIENTRY DllMain(HANDLE handle, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(startupThread), nullptr, 0, nullptr);
	}

	return TRUE;
}