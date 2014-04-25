#include <Foundation/Strings/Implementation/StringBase.h>

namespace ezInternal
{
  template <typename T, bool isString>
  struct HashHelperImpl
  {
    static ezUInt32 Hash(const T& value);
  };

  template <typename T>
  struct HashHelperImpl<T, true>
  {
    template <class Derived>
    EZ_FORCE_INLINE static ezUInt32 Hash(const ezStringBase<Derived>& string)
    {
      return ezHashing::MurmurHash((void*)string.InternalGetData(), string.InternalGetElementCount());
    }
  };

  template <typename T, bool isString>
  EZ_FORCE_INLINE ezUInt32 HashHelperImpl<T, isString>::Hash(const T& value)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(isString, "ezHashHelper is not implemented for the given type.");
    return 0;
  }
}

template <typename T>
EZ_FORCE_INLINE ezUInt32 ezHashHelper<T>::Hash(const T& value)
{
  return ezInternal::HashHelperImpl<T, EZ_IS_DERIVED_FROM_STATIC(ezThisIsAString, T)>::Hash(value);
}

template <typename T>
EZ_FORCE_INLINE bool ezHashHelper<T>::Equal(const T& a, const T& b)
{
  return a == b;
}



template <>
struct ezHashHelper<ezUInt32>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezUInt32 value)
  {
    // knuth: multiplication by the golden ratio will minimize gaps in the hash space. 
    // 2654435761U: prime close to 2^32/phi with phi = golden ratio (sqrt(5) - 1) / 2
    return value * 2654435761U;
  }

  EZ_FORCE_INLINE static bool Equal(ezUInt32 a, ezUInt32 b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<ezInt32>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezInt32 value)
  {
    return value * 2654435761U;
  }

  EZ_FORCE_INLINE static bool Equal(ezInt32 a, ezInt32 b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<ezUInt64>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    return ezUInt32(value * 2654435761U);
  }

  EZ_FORCE_INLINE static bool Equal(ezUInt64 a, ezUInt64 b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<ezInt64>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezInt64 value)
  {
    return ezUInt32(value * 2654435761U);
  }

  EZ_FORCE_INLINE static bool Equal(ezInt64 a, ezInt64 b)
  {
    return a == b;
  }
};

template <>
struct ezHashHelper<const char*>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(const char* szValue)
  {
    return ezHashing::MurmurHash(szValue);
  }

  EZ_FORCE_INLINE static bool Equal(const char* a, const char* b)
  {
    return ezStringUtils::IsEqual(a, b);
  }
};

template <typename T>
struct ezHashHelper<T*>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(T* value)
  {
    return ezUInt32((reinterpret_cast<size_t>(value) << 4) * 2654435761U);
  }

  EZ_FORCE_INLINE static bool Equal(T* a, T* b)
  {
    return a == b;
  }
};

