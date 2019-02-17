#pragma once

#include <string>
#include <cstdint>

// sizeof(Address) should always be sizeof(void*).
class Address
{
public:
	Address();
	Address(void* addr);
	Address(uintptr_t addr);

	void set(void* ptr)
	{
		mPtr = ptr;
	}

	template <typename T>
	Address get(T offset) const
	{
		return Address(reinterpret_cast<uintptr_t>(mPtr + offset));
	}

	template <typename T>
	Address add(T offset) const
	{
		return Address(reinterpret_cast<uintptr_t>(mPtr + offset));
	}

	template <typename T>
	Address sub(T offset) const
	{
		return Address(static_cast<uintptr_t>(mPtr - offset));
	}

	template <typename T>
	T as() const
	{
		return static_cast<T>(mPtr);
	}

	template <typename T>
	T to() const
	{
		return *static_cast<T*>(mPtr);
	}

	void* ptr() const
	{
		return mPtr;
	}

	operator uintptr_t() const
	{
		return reinterpret_cast<uintptr_t>(mPtr);
	}

	operator void*() const
	{
		return mPtr;
	}

	bool operator ==(bool val)
	{
		return ((mPtr && val) || (!mPtr && !val));
	}

	bool operator !=(bool val)
	{
		return !(*this == val);
	}

	bool operator ==(uintptr_t val)
	{
		return (reinterpret_cast<uintptr_t>(mPtr) == val);
	}

	bool operator !=(uintptr_t val)
	{
		return !(*this == val);
	}

	bool operator ==(void* val)
	{
		return (mPtr == val);
	}

	bool operator !=(void* val)
	{
		return !(*this == val);
	}

private:
	void* mPtr;
};

typedef Address Offset;
