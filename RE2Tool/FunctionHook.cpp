#include "FunctionHook.h"

static bool gIsMinHookInitialized {false};

FunctionHook::FunctionHook(Address target, Address destination) : mTarget {0}, mDestination {0}, mOriginal {0}
{
	// Initialize MinHook if it hasn't been already.
	if (!gIsMinHookInitialized && MH_Initialize() == MH_OK)
	{
		gIsMinHookInitialized = true;
	}

	// Create and enable the hook, only setting our member variables if it was
	// successful.
	uintptr_t original {0};

	if (MH_CreateHook(target.as<LPVOID>(), destination.as<LPVOID>(), (LPVOID*)&original) == MH_OK && MH_EnableHook(target.as<LPVOID>()) == MH_OK)
	{
		mTarget = target;
		mDestination = destination;
		mOriginal = original;
		std::cout << "Hooked: {0x" << std::hex << std::uppercase << static_cast<uintptr_t>(target) << "}->{0x" << std::hex << std::uppercase << static_cast<uintptr_t>(destination) << std::endl;
	}
	else
	{
		std::cout << "Hooked failed: {0x" << std::hex << std::uppercase << static_cast<uintptr_t>(target) << "}->{0x" << std::hex << std::uppercase << static_cast<uintptr_t>(destination) << std::endl;
	}
}

FunctionHook::~FunctionHook()
{
	remove();
}

bool FunctionHook::remove()
{
	// Don't try to remove invalid hooks.
	if (mOriginal == 0)
	{
		return true;
	}

	// Disable then remove the hook.
	if (MH_DisableHook(reinterpret_cast<LPVOID>(mTarget)) != MH_OK || MH_RemoveHook(reinterpret_cast<LPVOID>(mTarget)) != MH_OK)
	{
		return false;
	}

	// Invalidate the members.
	mTarget = 0;
	mDestination = 0;
	mOriginal = 0;

	return true;
}