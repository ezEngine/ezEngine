#pragma once

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b() {}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool b)
{
  m_v.x = b;
  m_v.y = b;
  m_v.z = b;
  m_v.w = b;
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool x, bool y, bool z, bool w)
{
  m_v.x = x;
  m_v.y = y;
  m_v.z = z;
  m_v.w = w;
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(ezInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::GetComponent() const
{
  return (&m_v.x)[N];
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::x() const
{
  return m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::y() const
{
  return m_v.y;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::z() const
{
  return m_v.z;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::w() const
{
  return m_v.w;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::Get() const
{
  ezSimdVec4b result;

  const bool* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator&&(const ezSimdVec4b& rhs) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x && rhs.m_v.x;
  result.m_v.y = m_v.y && rhs.m_v.y;
  result.m_v.z = m_v.z && rhs.m_v.z;
  result.m_v.w = m_v.w && rhs.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator||(const ezSimdVec4b& rhs) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x || rhs.m_v.x;
  result.m_v.y = m_v.y || rhs.m_v.y;
  result.m_v.z = m_v.z || rhs.m_v.z;
  result.m_v.w = m_v.w || rhs.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!() const
{
  ezSimdVec4b result;
  result.m_v.x = !m_v.x;
  result.m_v.y = !m_v.y;
  result.m_v.z = !m_v.z;
  result.m_v.w = !m_v.w;

  return result;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AllSet()
{
  for (int i = 0; i < N; ++i)
  {
    if (!(&m_v.x)[i])
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AnySet()
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i])
      return true;
  }

  return false;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::NoneSet()
{
  return !AnySet<N>();
}

