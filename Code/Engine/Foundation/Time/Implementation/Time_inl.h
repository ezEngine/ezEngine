#pragma once

#include <Foundation/Basics.h>

inline ezTime::ezTime(double fTime) : m_fTime(fTime)
{
}

inline ezTime::ezTime(const NanoSeconds& nanoSeconds)
{
  Set(nanoSeconds);
}

inline ezTime::ezTime(const MicroSeconds& microSeconds)
{
  Set(microSeconds);
}

inline ezTime::ezTime(const MilliSeconds& milliSeconds)
{
  Set(milliSeconds);
}

inline ezTime::ezTime(const Seconds& seconds)
{
  Set(seconds);
}

inline void ezTime::Set(const NanoSeconds& seconds)
{
  m_fTime = seconds.m_fTime * 0.000000001;
}

inline void ezTime::Set(const MicroSeconds& seconds)
{
  m_fTime = seconds.m_fTime * 0.000001;
}

inline void ezTime::Set(const MilliSeconds& seconds)
{
  m_fTime = seconds.m_fTime * 0.001;
}

inline void ezTime::Set(const Seconds& seconds)
{
  m_fTime = seconds.m_fTime;
}

inline float ezTime::AsFloat() const
{
  return static_cast<float>(m_fTime);
}

inline double ezTime::GetNanoSeconds() const
{
  return m_fTime * 1000000000.0;
}

inline double ezTime::GetMicroSeconds() const
{
  return m_fTime * 1000000.0;
}

inline double ezTime::GetMilliSeconds() const
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

