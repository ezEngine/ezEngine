#include <Foundation/Strings/Implementation/StringBase.h>

/*
 * HashHelper<T>
 */
template <typename T, bool isString>
struct ezHashHelperImpl
{
  static ezUInt32 Hash(const T& value);
};

template <typename T>
EZ_FORCE_INLINE ezUInt32 ezHashHelper<T>::Hash(const T& value)
{
  return ezHashHelperImpl<T, (ezConversionTest<T*, ezThisIsAString*>::exists == 1)>::Hash(value);
}

template <typename T, bool isString>
EZ_FORCE_INLINE ezUInt32 ezHashHelperImpl<T, isString>::Hash(const T& value)
{
  EZ_CHECK_AT_COMPILETIME_MSG((ezConversionTest<T*, ezThisIsAString*>::exists == 0), "Implementation error: strings should be not hashed here");
  return ezHashing::MurmurHash(&value, sizeof(T));
}

template <>
struct ezHashHelperImpl<ezUInt32, false>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezUInt32 value)
  {
    // knuth: multiplication by the golden ratio will minimize gaps in the hash space. 
    // 2654435761U: prime close to 2^32/phi with phi = golden ratio (sqrt(5) - 1) / 2
    return value * 2654435761U;
  }
};

template <>
struct ezHashHelperImpl<ezInt32, false>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezInt32 value)
  {
    return value * 2654435761U;
  }
};

template <>
struct ezHashHelperImpl<ezUInt64, false>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    return ezUInt32(value * 2654435761U);
  }
};

template <typename T>
struct ezHashHelperImpl<T, true>
{
  template <class Derived>
  EZ_FORCE_INLINE static ezUInt32 Hash(const ezStringBase<Derived>& string)
  {
    return ezHashing::MurmurHash((void*)string.InternalGetData(), string.InternalGetElementCount());
  }

  // for rehashing
  EZ_FORCE_INLINE static ezUInt32 Hash(ezUInt32 value)
  {
    return value * 2654435761U;
  }
};

