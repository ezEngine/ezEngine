
namespace ezInternal
{
  enum 
  {
    MURMUR_M = 0x5bd1e995,
    MURMUR_R = 24
  };

  template <size_t N>
  EZ_FORCE_INLINE ezUInt32 CompileTimeMurmurHash(ezUInt32 hash, const char* str, size_t i)
  {
    return CompileTimeMurmurHash<N-4>(CompileTimeMurmurHash<4>(hash, str, i), str, i+4);
  }

  template <>
  EZ_FORCE_INLINE ezUInt32 CompileTimeMurmurHash<4>(ezUInt32 hash, const char* str, size_t i)
  {
    ezUInt32 k = ((str[i+0]) | ((str[i+1])<<8) | ((str[i+2]) << 16) | ((str[i+3]) << 24));
    k *= MURMUR_M;
    k ^= (k >> MURMUR_R);
    k *= MURMUR_M;
    return (hash * MURMUR_M) ^ k;
  }

  template <>
  EZ_FORCE_INLINE ezUInt32 CompileTimeMurmurHash<3>(ezUInt32 hash, const char* str, size_t i)
  {
    return (hash ^ (str[i+2] << 16) ^ (str[i+1] << 8) ^ (str[i+0])) * MURMUR_M;
  }

  template <>
  EZ_FORCE_INLINE ezUInt32 CompileTimeMurmurHash<2>(ezUInt32 hash, const char* str, size_t i)
  {
    return (hash ^ (str[i+1] << 8) ^ (str[i])) * MURMUR_M;
  }

  template <>
  EZ_FORCE_INLINE ezUInt32 CompileTimeMurmurHash<1>(ezUInt32 hash, const char* str, size_t i)
  {
    return (hash ^ (str[i])) * MURMUR_M;
  }

  template <>
  EZ_FORCE_INLINE ezUInt32 CompileTimeMurmurHash<0>(ezUInt32 hash, const char* str, size_t i)
  {
    return hash;
  }
}

template <size_t N>
EZ_FORCE_INLINE ezUInt32 ezHashing::MurmurHash(const char (&str)[N], ezUInt32 uiSeed)
{
  const ezUInt32 uiStrlen = (ezUInt32)(N - 1);
  ezUInt32 h = ezInternal::CompileTimeMurmurHash<N - 1>(uiSeed ^ uiStrlen, str, 0);
  h ^= h >> 13;
  h *= ezInternal::MURMUR_M;
  h ^= h >> 15;
  return h;
}

EZ_FORCE_INLINE ezUInt32 ezHashing::MurmurHash(StringWrapper str, ezUInt32 uiSeed)
{
  return MurmurHash(str.m_str, std::strlen(str.m_str), uiSeed);
}

