#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>

template <typename T>
ezHashableStruct<T>::ezHashableStruct()
{
  ezMemoryUtils::ZeroFill<T>(static_cast<T*>(this), 1);
}

template <typename T>
ezUInt32 ezHashableStruct<T>::CalculateHash() const
{
  return ezHashingUtils::xxHash32(this, sizeof(T));
}

