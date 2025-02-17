#pragma once

#include <Foundation/Basics.h>

constexpr EZ_ALWAYS_INLINE ezTime::ezTime(double fTime)
  : m_fTime(fTime)
{
}

constexpr EZ_ALWAYS_INLINE float ezTime::AsFloatInSeconds() const
{
  return static_cast<float>(m_fTime);
}

constexpr EZ_ALWAYS_INLINE double ezTime::GetNanoseconds() const
{
  return m_fTime * 1000000000.0;
}

constexpr EZ_ALWAYS_INLINE double ezTime::GetMicroseconds() const
{
  return m_fTime * 1000000.0;
}

constexpr EZ_ALWAYS_INLINE double ezTime::GetMilliseconds() const
{
  return m_fTime * 1000.0;
}

constexpr EZ_ALWAYS_INLINE double ezTime::GetSeconds() const
{
  return m_fTime;
}

constexpr EZ_ALWAYS_INLINE double ezTime::GetMinutes() const
{
  return m_fTime / 60.0;
}

constexpr EZ_ALWAYS_INLINE double ezTime::GetHours() const
{
  return m_fTime / (60.0 * 60.0);
}

constexpr EZ_ALWAYS_INLINE void ezTime::operator-=(const ezTime& other)
{
  m_fTime -= other.m_fTime;
}

constexpr EZ_ALWAYS_INLINE void ezTime::operator+=(const ezTime& other)
{
  m_fTime += other.m_fTime;
}

constexpr EZ_ALWAYS_INLINE void ezTime::operator*=(double fFactor)
{
  m_fTime *= fFactor;
}

constexpr EZ_ALWAYS_INLINE void ezTime::operator/=(double fFactor)
{
  m_fTime /= fFactor;
}

constexpr EZ_ALWAYS_INLINE ezTime ezTime::operator-() const
{
  return ezTime(-m_fTime);
}

constexpr EZ_ALWAYS_INLINE ezTime ezTime::operator-(const ezTime& other) const
{
  return ezTime(m_fTime - other.m_fTime);
}

constexpr EZ_ALWAYS_INLINE ezTime ezTime::operator+(const ezTime& other) const
{
  return ezTime(m_fTime + other.m_fTime);
}

constexpr EZ_ALWAYS_INLINE ezTime operator*(const ezTime& t, double f)
{
  return ezTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr EZ_ALWAYS_INLINE ezTime operator*(double f, const ezTime& t)
{
  return ezTime::MakeFromSeconds(t.GetSeconds() * f);
}

constexpr EZ_ALWAYS_INLINE ezTime operator*(const ezTime& f, const ezTime& t)
{
  return ezTime::MakeFromSeconds(t.GetSeconds() * f.GetSeconds());
}

constexpr EZ_ALWAYS_INLINE ezTime operator/(const ezTime& t, double f)
{
  return ezTime::MakeFromSeconds(t.GetSeconds() / f);
}

constexpr EZ_ALWAYS_INLINE ezTime operator/(double f, const ezTime& t)
{
  return ezTime::MakeFromSeconds(f / t.GetSeconds());
}

constexpr EZ_ALWAYS_INLINE ezTime operator/(const ezTime& f, const ezTime& t)
{
  return ezTime::MakeFromSeconds(f.GetSeconds() / t.GetSeconds());
}
