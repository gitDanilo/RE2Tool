#pragma once

#include <iostream>
#include <Windows.h>
#include <MinHook.h>
#include <cstdint>
#include "Address.h"

class FunctionHook
{
public:
	FunctionHook() = delete;
	FunctionHook(const FunctionHook& other) = delete;
	FunctionHook(FunctionHook&& other) = delete;
	FunctionHook(Address target, Address destination);
	virtual ~FunctionHook();

	// Called automatically by the destructor, but you can call it explicitly
	// if you need to remove the hook.
	bool remove();

	auto getOriginal() const
	{
		return mOriginal;
	}

	template <typename T>
	T* getOriginal() const
	{
		return reinterpret_cast<T*>(mOriginal);
	}

	auto isValid() const
	{
		return mOriginal != 0;
	}

	FunctionHook& operator=(const FunctionHook& other) = delete;
	FunctionHook& operator=(FunctionHook&& other) = delete;

private:
	uintptr_t mTarget {0};
	uintptr_t mDestination {0};
	uintptr_t mOriginal {0};
};