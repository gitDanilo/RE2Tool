#include "VirtualData.h"

template class VirtualData<BYTE*>;
template class VirtualData<BYTE>;
template class VirtualData<WORD>;
template class VirtualData<SHORT>;
template class VirtualData<DWORD>;
template class VirtualData<INT>;
template class VirtualData<QWORD>;
template class VirtualData<INT64>;
template class VirtualData<FLOAT>;

template<typename T>
VirtualData<T>::VirtualData()
{
	this->BaseAddr = nullptr;
	this->DataAddr = nullptr;
}

template<typename T>
VirtualData<T>::VirtualData(BYTE* BaseAddr, std::vector<DWORD> OffsetList)
{
	this->BaseAddr = BaseAddr;
	this->OffsetList = OffsetList;
	this->DataAddr = nullptr;
}

template<typename T>
VirtualData<T>::~VirtualData()
{

}

template<typename T>
void VirtualData<T>::SetDataPtr(BYTE* BaseAddr, std::vector<DWORD> OffsetList)
{
	this->BaseAddr = BaseAddr;
	this->OffsetList = OffsetList;
	this->DataAddr = nullptr;
}

template<typename T>
bool VirtualData<T>::GetData(T &Data)
{
	if (DataAddr == nullptr)
	{
		DataAddr = memutil::ReadMultiLvlPtr(BaseAddr, OffsetList);
		if (DataAddr == nullptr)
			return false;
	}
	memcpy(&Data, DataAddr, sizeof(T));
	return true;
}

template<typename T>
DWORD VirtualData<T>::GetDataSize()
{
	return sizeof(T);
}

template<typename T>
BYTE* VirtualData<T>::GetDataAddr()
{
	return this->DataAddr;
}

template<typename T>
void VirtualData<T>::InvalidateDataAddr()
{
	this->DataAddr = nullptr;
}

DEFINE_GETDATATYPE(BYTE*, DataT::ptr     )
DEFINE_GETDATATYPE(BYTE , DataT::uint_8  )
DEFINE_GETDATATYPE(WORD , DataT::uint_16 )
DEFINE_GETDATATYPE(SHORT, DataT::int_16  )
DEFINE_GETDATATYPE(DWORD, DataT::uint_32 )
DEFINE_GETDATATYPE(INT  , DataT::int_32  )
DEFINE_GETDATATYPE(QWORD, DataT::uint_64 )
DEFINE_GETDATATYPE(INT64, DataT::int_64  )
DEFINE_GETDATATYPE(FLOAT, DataT::float_32)
