
namespace ezInternal
{
  constexpr ezUInt32 MURMUR_M = 0x5bd1e995;
  constexpr ezUInt32 MURMUR_R = 24;

  template <size_t N, size_t Loop>
  struct CompileTimeMurmurHash
  {
    constexpr EZ_ALWAYS_INLINE ezUInt32 operator() (ezUInt32 hash, const char(&str)[N], size_t i) const
    {
      return CompileTimeMurmurHash<N, Loop - 4>()(CompileTimeMurmurHash<N, 4>()(hash, str, i), str, i + 4);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 4>
  {
    static constexpr EZ_ALWAYS_INLINE ezUInt32 helper(ezUInt32 k)
    {
      return (k ^ (k >> MURMUR_R)) * MURMUR_M;
    }

    constexpr EZ_ALWAYS_INLINE ezUInt32 operator() (ezUInt32 hash, const char(&str)[N], size_t i) const
    {
      // In C++11 constexpr local variables are not allowed. Need to express the following without "ezUInt32 k"
      // (this restriction is lifted in C++14's generalized constexpr)
      //ezUInt32 k = ((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24));
      //k *= MURMUR_M;
      //k ^= (k >> MURMUR_R);
      //k *= MURMUR_M;
      //return (hash * MURMUR_M) ^ k;

      return (hash * MURMUR_M) ^
        helper( ((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24))
                * MURMUR_M );
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 3>
  {
    constexpr EZ_ALWAYS_INLINE ezUInt32 operator()(ezUInt32 hash, const char(&str)[N], size_t i) const
    {
      return (hash ^ (str[i + 2] << 16) ^ (str[i + 1] << 8) ^ (str[i + 0])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 2>
  {
    constexpr EZ_ALWAYS_INLINE ezUInt32 operator()(ezUInt32 hash, const char(&str)[N], size_t i) const
    {
      return (hash ^ (str[i + 1] << 8) ^ (str[i])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 1>
  {
    constexpr EZ_ALWAYS_INLINE ezUInt32 operator()(ezUInt32 hash, const char(&str)[N], size_t i) const
    {
      return (hash ^ (str[i])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 0>
  {
    constexpr EZ_ALWAYS_INLINE ezUInt32 operator()(ezUInt32 hash, const char(&str)[N], size_t i) const
    {
      return hash;
    }
  };

  constexpr ezUInt32 rightShift_and_xorWithPrevSelf(ezUInt32 h, ezUInt32 shift)
  {
    return h ^ (h >> shift);
  }
}

template <size_t N>
constexpr EZ_ALWAYS_INLINE ezUInt32 ezHashing::MurmurHash32String(const char (&str)[N], ezUInt32 uiSeed)
{
  // In C++11 constexpr local variables are not allowed. Need to express the following without "ezUInt32 h"
  // (this restriction is lifted in C++14's generalized constexpr)
  //const ezUInt32 uiStrlen = (ezUInt32)(N - 1);
  //ezUInt32 h = ezInternal::CompileTimeMurmurHash<N - 1>(uiSeed ^ uiStrlen, str, 0);
  //h ^= h >> 13;
  //h *= ezInternal::MURMUR_M;
  //h ^= h >> 15;
  //return h;

  return ezInternal::rightShift_and_xorWithPrevSelf(
           ezInternal::rightShift_and_xorWithPrevSelf(
             ezInternal::CompileTimeMurmurHash<N, N - 1>()(uiSeed ^ static_cast<ezUInt32>(N - 1), str, 0),
           13) * ezInternal::MURMUR_M,
         15);
}

EZ_ALWAYS_INLINE ezUInt32 ezHashing::MurmurHash32String(StringWrapper str, ezUInt32 uiSeed)
{
  return MurmurHash32(str.m_str, std::strlen(str.m_str), uiSeed);
}

