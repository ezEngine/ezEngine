#pragma once

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b() {}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool b)
{
  m_v.x = b ? 0xFFFFFFFF : 0;
  m_v.y = b ? 0xFFFFFFFF : 0;
  m_v.z = b ? 0xFFFFFFFF : 0;
  m_v.w = b ? 0xFFFFFFFF : 0;
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool x, bool y, bool z, bool w)
{
  m_v.x = x ? 0xFFFFFFFF : 0;
  m_v.y = y ? 0xFFFFFFFF : 0;
  m_v.z = z ? 0xFFFFFFFF : 0;
  m_v.w = w ? 0xFFFFFFFF : 0;
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(ezInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::GetComponent() const
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

EZ_ALWAYS_INLINE bool ezSimdVec4b::x() const
{
  return m_v.x != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::y() const
{
  return m_v.y != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::z() const
{
  return m_v.z != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::w() const
{
  return m_v.w != 0;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::Get() const
{
  ezSimdVec4b result;

  const ezUInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator&&(const ezSimdVec4b& rhs) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x & rhs.m_v.x;
  result.m_v.y = m_v.y & rhs.m_v.y;
  result.m_v.z = m_v.z & rhs.m_v.z;
  result.m_v.w = m_v.w & rhs.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator||(const ezSimdVec4b& rhs) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x | rhs.m_v.x;
  result.m_v.y = m_v.y | rhs.m_v.y;
  result.m_v.z = m_v.z | rhs.m_v.z;
  result.m_v.w = m_v.w | rhs.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!() const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x ^ 0xFFFFFFFF;
  result.m_v.y = m_v.y ^ 0xFFFFFFFF;
  result.m_v.z = m_v.z ^ 0xFFFFFFFF;
  result.m_v.w = m_v.w ^ 0xFFFFFFFF;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator==(const ezSimdVec4b& rhs) const
{
  return !(*this != rhs);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!=(const ezSimdVec4b& rhs) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x ^ rhs.m_v.x;
  result.m_v.y = m_v.y ^ rhs.m_v.y;
  result.m_v.z = m_v.z ^ rhs.m_v.z;
  result.m_v.w = m_v.w ^ rhs.m_v.w;

  return result;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AllSet() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!(&m_v.x)[i])
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AnySet() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i])
      return true;
  }

  return false;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::NoneSet() const
{
  return !AnySet<N>();
}

// static
EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::Select(const ezSimdVec4b& cmp, const ezSimdVec4b& ifTrue, const ezSimdVec4b& ifFalse)
{
  ezSimdVec4b result;
  result.m_v.x = (cmp.m_v.x != 0) ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = (cmp.m_v.y != 0) ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = (cmp.m_v.z != 0) ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = (cmp.m_v.w != 0) ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}
