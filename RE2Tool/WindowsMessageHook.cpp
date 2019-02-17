#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "WindowsMessageHook.h"

static WindowsMessageHook* gWindowsMessageHook {nullptr};
std::recursive_mutex gProcMutex {};

LRESULT WINAPI WindowProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::lock_guard<std::recursive_mutex> guard(gProcMutex);

	if (gWindowsMessageHook == nullptr)
	{
		return 0;
	}

	// Call our onMessage callback.
	auto& _OnMessage = gWindowsMessageHook->OnMessage;

	if (_OnMessage)
	{
		// If it returns false we don't call the original window procedure.
		if (!_OnMessage(wnd, message, wParam, lParam))
		{
			return DefWindowProc(wnd, message, wParam, lParam);
		}
	}

	// Call the original message procedure.
	return CallWindowProc(gWindowsMessageHook->getOriginal(), wnd, message, wParam, lParam);
}

WindowsMessageHook::WindowsMessageHook(HWND wnd) : mWnd {wnd}, mOriginalProc {nullptr}
{
	std::cout << "Initializing WindowsMessageHook..." << std::endl;

	gWindowsMessageHook = this;

	// Save the original window procedure.
	mOriginalProc = (WNDPROC)GetWindowLongPtr(mWnd, GWLP_WNDPROC);

	// Set it to our "hook" procedure.
	SetWindowLongPtr(mWnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);

	std::cout << "Hooked Windows message handler." << std::endl;
}

WindowsMessageHook::~WindowsMessageHook()
{
	std::lock_guard<std::recursive_mutex> guard(gProcMutex);

	remove();
	gWindowsMessageHook = nullptr;
}

bool WindowsMessageHook::remove()
{
	// Don't attempt to restore invalid original window procedures.
	if (mOriginalProc == nullptr || mWnd == nullptr)
	{
		return true;
	}

	// Restore the original window procedure.
	SetWindowLongPtr(mWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(mOriginalProc));

	// Invalidate this message hook.
	mWnd = nullptr;
	mOriginalProc = nullptr;

	return true;
}