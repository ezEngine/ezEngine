#include <Foundation/Algorithm/Hashing.h>
#include <Foundation/Memory/MemoryUtils.h>

template<typename T> ezHashableStruct<T>::ezHashableStruct()
{
  ezMemoryUtils::ZeroFill<T>(static_cast<T*>(this));
}

template<typename T> ezUInt32 ezHashableStruct<T>::CalculateHash() const
{
  return ezHashing::CRC32Hash(this, sizeof(T));
}

