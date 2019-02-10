#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "WindowsMessageHook.h"

static WindowsMessageHook* gWindowsMessageHook {nullptr};
std::recursive_mutex gProcMutex {};

LRESULT WINAPI WindowProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::lock_guard<std::recursive_mutex> guard (gProcMutex);

	if (gWindowsMessageHook == nullptr)
	{
		return 0;
	}

	// Call our onMessage callback.
	auto& onMessage = gWindowsMessageHook->onMessage;

	if (onMessage)
	{
		// If it returns false we don't call the original window procedure.
		if (!onMessage(wnd, message, wParam, lParam))
		{
			return DefWindowProc(wnd, message, wParam, lParam);
		}
	}

	// Call the original message procedure.
	return CallWindowProc(gWindowsMessageHook->getOriginal(), wnd, message, wParam, lParam);
}

WindowsMessageHook::WindowsMessageHook(HWND wnd)
	: m_wnd {wnd},
	m_originalProc {nullptr}
{
	std::cout << "Initializing WindowsMessageHook" << std::endl;

	gWindowsMessageHook = this;

	// Save the original window procedure.
	m_originalProc = (WNDPROC)GetWindowLongPtr(m_wnd, GWLP_WNDPROC);

	// Set it to our "hook" procedure.
	SetWindowLongPtr(m_wnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);

	std::cout << "Hooked Windows message handler" << std::endl;
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
	if (m_originalProc == nullptr || m_wnd == nullptr)
	{
		return true;
	}

	// Restore the original window procedure.
	SetWindowLongPtr(m_wnd, GWLP_WNDPROC, (LONG_PTR)m_originalProc);

	// Invalidate this message hook.
	m_wnd = nullptr;
	m_originalProc = nullptr;

	return true;
}