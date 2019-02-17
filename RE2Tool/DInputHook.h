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
		mIsIgnoringInput = true;
	}

	void acknowledgeInput()
	{
		mIsIgnoringInput = false;
	}

	auto isIgnoringInput() const
	{
		return mIsIgnoringInput;
	}

	auto isValid() const
	{
		return mGetDeviceStateHook->isValid();
	}

	void setWindow(HWND wnd)
	{
		mWnd = wnd;
	}

	DInputHook& operator=(const DInputHook& other) = delete;
	DInputHook& operator=(DInputHook&& other) = delete;

private:
	HWND mWnd;

	std::unique_ptr<FunctionHook> mGetDeviceStateHook;

	bool mIsIgnoringInput;
	bool mDoOnce;

	bool hook();

	HRESULT GetDeviceState_Internal(IDirectInputDevice* device, DWORD size, LPVOID data);
	static HRESULT WINAPI GetDeviceState(IDirectInputDevice* device, DWORD size, LPVOID data);
};
