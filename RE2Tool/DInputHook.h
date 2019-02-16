#pragma once

#include <memory>
#include <vector>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "REFramework.h"
#include "FunctionHook.h"

class DInputHook
{
public:
	DInputHook() = delete;
	DInputHook(const DInputHook& other) = delete;
	DInputHook(DInputHook&& other) = delete;
	DInputHook(HWND wnd);
	virtual ~DInputHook();

	void ignoreInput()
	{
		m_isIgnoringInput = true;
	}

	void acknowledgeInput()
	{
		m_isIgnoringInput = false;
	}

	auto isIgnoringInput() const
	{
		return m_isIgnoringInput;
	}

	auto isValid() const
	{
		return m_GetDeviceStateHook->isValid();
	}

	void setWindow(HWND wnd)
	{
		m_wnd = wnd;
	}

	DInputHook& operator=(const DInputHook& other) = delete;
	DInputHook& operator=(DInputHook&& other) = delete;

private:
	HWND m_wnd;

	std::unique_ptr<FunctionHook> m_GetDeviceStateHook;

	bool m_isIgnoringInput;
	bool m_doOnce;

	bool hook();

	HRESULT GetDeviceState_Internal(IDirectInputDevice* device, DWORD size, LPVOID data);
	static HRESULT WINAPI GetDeviceState(IDirectInputDevice* device, DWORD size, LPVOID data);
};
