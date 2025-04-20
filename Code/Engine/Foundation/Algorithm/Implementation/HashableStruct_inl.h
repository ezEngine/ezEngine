#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Memory/MemoryUtils.h>

template <typename T>
EZ_ALWAYS_INLINE ezHashableStruct<T>::ezHashableStruct()
{
  ezMemoryUtils::ZeroFill<T>(static_cast<T*>(this), 1);
}

template <typename T>
EZ_ALWAYS_INLINE ezHashableStruct<T>::ezHashableStruct(const ezHashableStruct<T>& other)
{
  ezMemoryUtils::RawByteCopy(this, &other, sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezHashableStruct<T>::operator=(const ezHashableStruct<T>& other)
{
  if (this != &other)
  {
    ezMemoryUtils::RawByteCopy(this, &other, sizeof(T));
  }
}
template <typename T>
bool ezHashableStruct<T>::operator==(const ezHashableStruct<T>& other) const
{
  return ezMemoryUtils::RawByteCompare(this, &other, sizeof(T)) == 0;
}

template <typename T>
bool ezHashableStruct<T>::operator!=(const ezHashableStruct<T>& other) const
{
  return ezMemoryUtils::RawByteCompare(this, &other, sizeof(T)) != 0;
}

template <typename T>
bool ezHashableStruct<T>::operator<(const ezHashableStruct<T>& other) const
{
  return ezMemoryUtils::RawByteCompare(this, &other, sizeof(T)) < 0;
}

template <typename T>
EZ_ALWAYS_INLINE ezUInt32 ezHashableStruct<T>::CalculateHash() const
{
  return ezHashingUtils::xxHash32(this, sizeof(T));
}
