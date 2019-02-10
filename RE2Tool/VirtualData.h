#pragma once

#include "MemoryUtil.h"

enum class DataT
{
	ptr, uint_8, uint_16, int_16, uint_32, int_32, uint_64, int_64, float_32
};

// https://stackoverflow.com/questions/1055452/c-get-name-of-type-in-template
#define DEFINE_GETDATATYPE(type, type_id) template<> DataT VirtualData<type>::GetDataType() { return type_id; }

template<typename T>
class VirtualData
{
private:
	BYTE* BaseAddr;
	BYTE* DataAddr;
	std::vector<DWORD> OffsetList;
public:
	VirtualData();
	VirtualData(BYTE* BaseAddr, std::vector<DWORD> OffsetList);
	~VirtualData();
	void SetDataPtr(BYTE* BaseAddr, std::vector<DWORD> OffsetList);
	bool GetData(T &Data);
	DWORD GetDataSize();
	BYTE* GetDataAddr();
	DataT GetDataType();
	void InvalidateDataAddr();
};
