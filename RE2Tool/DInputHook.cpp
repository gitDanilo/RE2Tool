#define INITGUID

#include "DInputHook.h"

static DInputHook* gDInputHook {nullptr};

DInputHook::DInputHook(HWND wnd)
	: m_wnd {wnd},
	m_isIgnoringInput {false},
	m_doOnce {true}
{
	if (gDInputHook == nullptr)
	{
		if (hook())
		{
			std::cout << "DInputHook hooked successfully." << std::endl;
			gDInputHook = this;
		}
		else
		{
			std::cout << "DInputHook failed to hook." << std::endl;
		}
	}
}

DInputHook::~DInputHook()
{
	// Explicitly unhook the methods we hooked so we can reset g_dinputHook.
	m_GetDeviceStateHook.reset();

	gDInputHook = nullptr;
}

bool DInputHook::hook()
{
	//spdlog::info("Entering DInputHook::hook().");

	// All we do here is create an IDirectInputDevice so that we can get the
	// addresses of the methods we want to hook from its vtable.
	using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

	auto dinput8 = LoadLibrary("dinput8.dll");
	auto dinput8Create = (DirectInput8CreateFn)GetProcAddress(dinput8, "DirectInput8Create");

	if (dinput8Create == nullptr)
	{
		//spdlog::info("Failed to find DirectInput8Create.");
		return false;
	}

	//spdlog::info("Got DirectInput8Create {:p}", (void*)dinput8Create);

	auto instance = (HINSTANCE)GetModuleHandle(nullptr);
	IDirectInput* dinput {nullptr};

	if (FAILED(dinput8Create(instance, DIRECTINPUT_VERSION, IID_IDirectInput8W, (LPVOID*)&dinput, nullptr)))
	{
		//spdlog::info("Failed to create IDirectInput.");
		return false;
	}

	//spdlog::info("Got IDirectInput {:p}", (void*)dinput);

	IDirectInputDevice* device {nullptr};

	// Keyboard
	if (FAILED(dinput->CreateDevice(GUID_SysKeyboard, &device, nullptr)))
	{
		//spdlog::info("Failed to create IDirectInputDevice.");
		dinput->Release();
		return false;
	}

	auto getDeviceState = (*(uintptr_t**)device)[9];

	m_GetDeviceStateHook = std::make_unique<FunctionHook>(getDeviceState, (uintptr_t)&DInputHook::GetDeviceState);

	device->Release();
	dinput->Release();

	return m_GetDeviceStateHook->isValid();
}

HRESULT DInputHook::GetDeviceState_Internal(IDirectInputDevice* device, DWORD size, LPVOID data)
{
	auto OriginalGetDeviceState = (decltype(DInputHook::GetDeviceState)*)m_GetDeviceStateHook->getOriginal();

	// If we are ignoring input then we call the original to remove buffered    
	// input events from the devices queue without modifying the out parameters.
	//if (m_isIgnoringInput || m_doOnce)
	//{
	//	device->Unacquire();
	//	device->SetCooperativeLevel(m_wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	//	device->Acquire();

	//	m_doOnce = false;
	//}

	auto res = OriginalGetDeviceState(device, size, data);

	// Feed keys back to the framework
	if (res == DI_OK && !m_isIgnoringInput && data != nullptr)
	{
		if (gREFramework != nullptr)
			gREFramework->OnDirectInputKeys(*(std::array<uint8_t, 256>*)data);
	}

	return res;
}

HRESULT WINAPI DInputHook::GetDeviceState(IDirectInputDevice* device, DWORD size, LPVOID data)
{
	return gDInputHook->GetDeviceState_Internal(device, size, data);
}

