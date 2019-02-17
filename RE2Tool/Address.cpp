#include <cstdio>
#include "Address.h"

using namespace std;

Address::Address() : mPtr(nullptr)
{

}

Address::Address(void* addr) : mPtr(addr)
{

}

Address::Address(uintptr_t addr) : mPtr(reinterpret_cast<void*>(addr))
{

}