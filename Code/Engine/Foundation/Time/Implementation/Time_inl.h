#pragma once

#include <Foundation/Basics.h>

inline ezTime::ezTime(double fTime) : m_fTime(fTime)
{
}

inline void ezTime::SetZero()
{
  m_fTime = 0.0;
}

inline float ezTime::AsFloat() const
{
  return static_cast<float>(m_fTime);
}

inline double ezTime::GetNanoseconds() const
{
  return m_fTime * 1000000000.0;
}

inline double ezTime::GetMicroseconds() const
{
  return m_fTime * 1000000.0;
}

inline double ezTime::GetMilliseconds() const
{
  return m_fTime * 1000.0;
}

inline double ezTime::GetSeconds() const
{
  return m_fTime;
}

inline void ezTime::operator -= (const ezTime& other)
{
  m_fTime -= other.m_fTime;
}

inline void ezTime::operator += (const ezTime& other)
{
  m_fTime += other.m_fTime;
}

inline ezTime ezTime::operator - (const ezTime& other) const
{
  return ezTime(m_fTime - other.m_fTime);
}

inline ezTime ezTime::operator + (const ezTime& other) const
{
  return ezTime(m_fTime + other.m_fTime);
}

EZ_FORCE_INLINE ezTime operator* (ezTime t, double f)
{
  return ezTime::Seconds(t.GetSeconds() * f);
}

EZ_FORCE_INLINE ezTime operator* (double f, ezTime t)
{
  return ezTime::Seconds(t.GetSeconds() * f);
}

EZ_FORCE_INLINE ezTime operator/ (ezTime t, double f)
{
  return ezTime::Seconds(t.GetSeconds() / f);
}

EZ_FORCE_INLINE ezTime operator/ (double f, ezTime t)
{
  return ezTime::Seconds(t.GetSeconds() / f);
}

