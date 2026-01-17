#pragma once

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide() {}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool b)
{
  m_v.x = b ? 0xFFFFFFFFFFFFFFFF : 0;
  m_v.y = b ? 0xFFFFFFFFFFFFFFFF : 0;
  m_v.z = b ? 0xFFFFFFFFFFFFFFFF : 0;
  m_v.w = b ? 0xFFFFFFFFFFFFFFFF : 0;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool x, bool y, bool z, bool w)
{
  m_v.x = x ? 0xFFFFFFFFFFFFFFFF : 0;
  m_v.y = y ? 0xFFFFFFFFFFFFFFFF : 0;
  m_v.z = z ? 0xFFFFFFFFFFFFFFFF : 0;
  m_v.w = w ? 0xFFFFFFFFFFFFFFFF : 0;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(ezInternal::QuadBoolWide v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::GetComponent() const
{
  if constexpr (N == 0)
  {
    return m_v.x != 0;
  }
  else if constexpr (N == 1)
  {
    return m_v.y != 0;
  }
  else if constexpr (N == 2)
  {
    return m_v.z != 0;
  }
  else if constexpr (N == 3)
  {
    return m_v.w != 0;
  }
  else
  {
    return m_v.w != 0;
  }
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::x() const
{
  return m_v.x != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::y() const
{
  return m_v.y != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::z() const
{
  return m_v.z != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::w() const
{
  return m_v.w != 0;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::Get() const
{
  ezSimdVec4bWide result;

  const ezUInt64* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator&&(const ezSimdVec4bWide& rhs) const
{
  ezSimdVec4bWide result;
  result.m_v.x = m_v.x & rhs.m_v.x;
  result.m_v.y = m_v.y & rhs.m_v.y;
  result.m_v.z = m_v.z & rhs.m_v.z;
  result.m_v.w = m_v.w & rhs.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator||(const ezSimdVec4bWide& rhs) const
{
  ezSimdVec4bWide result;
  result.m_v.x = m_v.x | rhs.m_v.x;
  result.m_v.y = m_v.y | rhs.m_v.y;
  result.m_v.z = m_v.z | rhs.m_v.z;
  result.m_v.w = m_v.w | rhs.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!() const
{
  ezSimdVec4bWide result;
  result.m_v.x = m_v.x ^ 0xFFFFFFFFFFFFFFFF;
  result.m_v.y = m_v.y ^ 0xFFFFFFFFFFFFFFFF;
  result.m_v.z = m_v.z ^ 0xFFFFFFFFFFFFFFFF;
  result.m_v.w = m_v.w ^ 0xFFFFFFFFFFFFFFFF;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator==(const ezSimdVec4bWide& rhs) const
{
  return !(*this != rhs);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!=(const ezSimdVec4bWide& rhs) const
{
  ezSimdVec4bWide result;
  result.m_v.x = m_v.x ^ rhs.m_v.x;
  result.m_v.y = m_v.y ^ rhs.m_v.y;
  result.m_v.z = m_v.z ^ rhs.m_v.z;
  result.m_v.w = m_v.w ^ rhs.m_v.w;

  return result;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AllSet() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!(&m_v.x)[i])
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AnySet() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i])
      return true;
  }

  return false;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::NoneSet() const
{
  return !AnySet<N>();
}

// static
EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::Select(const ezSimdVec4bWide& cmp, const ezSimdVec4bWide& ifTrue, const ezSimdVec4bWide& ifFalse)
{
  ezSimdVec4bWide result;
  result.m_v.x = (cmp.m_v.x != 0) ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = (cmp.m_v.y != 0) ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = (cmp.m_v.z != 0) ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = (cmp.m_v.w != 0) ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}
