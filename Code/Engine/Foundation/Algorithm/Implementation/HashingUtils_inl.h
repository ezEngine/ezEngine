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
    EZ_ALWAYS_INLINE static ezUInt32 Hash(ezStringView sString)
    {
      return ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sString));
    }
  };

  template <typename T, bool isString>
  EZ_ALWAYS_INLINE ezUInt32 HashHelperImpl<T, isString>::Hash(const T& value)
  {
    static_assert(isString, "ezHashHelper is not implemented for the given type.");
    return 0;
  }
} // namespace ezInternal

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezUInt32 ezHashHelper<T>::Hash(const U& value)
{
  return ezInternal::HashHelperImpl<T, EZ_IS_DERIVED_FROM_STATIC(ezThisIsAString, T)>::Hash(value);
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE bool ezHashHelper<T>::Equal(const T& a, const U& b)
{
  return a == b;
}



template <>
struct ezHashHelper<ezUInt32>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezUInt32 value)
  {
    // Knuth: multiplication by the golden ratio will minimize gaps in the hash space.
    // 2654435761U: prime close to 2^32/phi with phi = golden ratio (sqrt(5) - 1) / 2
    return value * 2654435761U;
  }

  EZ_ALWAYS_INLINE static bool Equal(ezUInt32 a, ezUInt32 b) { return a == b; }
};

template <>
struct ezHashHelper<ezInt32>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezInt32 value) { return ezHashHelper<ezUInt32>::Hash(ezUInt32(value)); }

  EZ_ALWAYS_INLINE static bool Equal(ezInt32 a, ezInt32 b) { return a == b; }
};

template <>
struct ezHashHelper<ezUInt64>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    // boost::hash_combine.
    ezUInt32 a = ezUInt32(value >> 32);
    ezUInt32 b = ezUInt32(value);
    return a ^ (b + 0x9e3779b9 + (a << 6) + (b >> 2));
  }

  EZ_ALWAYS_INLINE static bool Equal(ezUInt64 a, ezUInt64 b) { return a == b; }
};

template <>
struct ezHashHelper<ezInt64>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezInt64 value) { return ezHashHelper<ezUInt64>::Hash(ezUInt64(value)); }

  EZ_ALWAYS_INLINE static bool Equal(ezInt64 a, ezInt64 b) { return a == b; }
};

template <>
struct ezHashHelper<const char*>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(const char* szValue)
  {
    return ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(szValue));
  }

  EZ_ALWAYS_INLINE static bool Equal(const char* a, const char* b) { return ezStringUtils::IsEqual(a, b); }
};

template <>
struct ezHashHelper<ezStringView>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(ezStringView sValue)
  {
    return ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sValue));
  }

  EZ_ALWAYS_INLINE static bool Equal(ezStringView a, ezStringView b) { return a == b; }
};

template <typename T>
struct ezHashHelper<T*>
{
  EZ_ALWAYS_INLINE static ezUInt32 Hash(T* value)
  {
#if EZ_ENABLED(EZ_PLATFORM_64BIT)
    return ezHashHelper<ezUInt64>::Hash(reinterpret_cast<ezUInt64>(value) >> 4);
#else
    return ezHashHelper<ezUInt32>::Hash(reinterpret_cast<ezUInt32>(value) >> 4);
#endif
  }

  EZ_ALWAYS_INLINE static bool Equal(T* a, T* b)
  {
    return a == b;
  }
};

template <size_t N>
constexpr EZ_ALWAYS_INLINE ezUInt64 ezHashingUtils::StringHash(const char (&str)[N], ezUInt64 uiSeed)
{
  return xxHash64String(str, uiSeed);
}

EZ_ALWAYS_INLINE ezUInt64 ezHashingUtils::StringHash(ezStringView sStr, ezUInt64 uiSeed)
{
  return xxHash64String(sStr, uiSeed);
}

constexpr EZ_ALWAYS_INLINE ezUInt32 ezHashingUtils::StringHashTo32(ezUInt64 uiHash)
{
  // just throw away the upper bits
  return static_cast<ezUInt32>(uiHash);
}

constexpr EZ_ALWAYS_INLINE ezUInt32 ezHashingUtils::CombineHashValues32(ezUInt32 ui0, ezUInt32 ui1)
{
  // See boost::hash_combine
  return ui0 ^ (ui1 + 0x9e3779b9 + (ui0 << 6) + (ui1 >> 2));
}
