

/*
 * HashHelper<T>
 */
template <typename T>
EZ_FORCE_INLINE ezUInt32 ezHashHelper<T>::Hash(T value)
{
  return ezHashing::MurmurHash(&value, sizeof(T));
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
};

template <>
struct ezHashHelper<ezInt32>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezInt32 value)
  {
    return value * 2654435761U;
  }
};

template <>
struct ezHashHelper<ezUInt64>
{
  EZ_FORCE_INLINE static ezUInt32 Hash(ezUInt64 value)
  {
    return ezUInt32(value * 2654435761U);
  }
};
