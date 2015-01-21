#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

static const ezInt64 EZ_INVALID_TIME_STAMP = 0x7FFFFFFFFFFFFFFFLL;

inline ezTimestamp::ezTimestamp()
{
  Invalidate();
}

inline ezTimestamp::ezTimestamp(ezInt64 iTimeValue, ezSIUnitOfTime::Enum unitOfTime) 
{
  SetInt64(iTimeValue, unitOfTime);
}

inline void ezTimestamp::Invalidate()
{
  m_iTimestamp = EZ_INVALID_TIME_STAMP;
}

inline bool ezTimestamp::IsValid() const
{
  return m_iTimestamp != EZ_INVALID_TIME_STAMP;
}

inline void ezTimestamp::operator+=(const ezTime& timeSpan)
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp += (ezInt64)timeSpan.GetMicroseconds();
}

inline void ezTimestamp::operator-=(const ezTime& timeSpan)
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  m_iTimestamp -= (ezInt64)timeSpan.GetMicroseconds();
}

inline const ezTime ezTimestamp::operator-(const ezTimestamp& other) const
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  EZ_ASSERT_DEBUG(other.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTime::Microseconds((double)(m_iTimestamp - other.m_iTimestamp)); 
}

inline const ezTimestamp ezTimestamp::operator+(const ezTime& timeSpan) const
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTimestamp(m_iTimestamp + (ezInt64)timeSpan.GetMicroseconds(), ezSIUnitOfTime::Microsecond);
}

inline const ezTimestamp ezTimestamp::operator-(const ezTime& timeSpan) const
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTimestamp(m_iTimestamp - (ezInt64)timeSpan.GetMicroseconds(), ezSIUnitOfTime::Microsecond);
}

inline const ezTimestamp operator+ (const ezTime& timeSpan, const ezTimestamp& timestamp)
{
  EZ_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTimestamp(timestamp.GetInt64(ezSIUnitOfTime::Microsecond) + (ezInt64)timeSpan.GetMicroseconds(), ezSIUnitOfTime::Microsecond);
}



inline ezUInt32 ezDateTime::GetYear() const
{
  return m_iYear;
}

inline void ezDateTime::SetYear(ezInt16 iYear)
{
  m_iYear = iYear;
}

inline ezUInt8 ezDateTime::GetMonth() const
{
  return m_uiMonth;
}

inline void ezDateTime::SetMonth(ezUInt8 uiMonth)
{
  m_uiMonth = ezMath::Clamp<ezUInt8>(uiMonth, 1, 12);
}

inline ezUInt8 ezDateTime::GetDay() const
{
  return m_uiDay;
}

inline void ezDateTime::SetDay(ezUInt8 uiDay)
{
  m_uiDay = ezMath::Clamp<ezUInt8>(uiDay, 1u, 31u);
}

inline ezUInt8 ezDateTime::GetHour() const
{
  return m_uiHour;
}

inline void ezDateTime::SetHour(ezUInt8 uiHour)
{
  m_uiHour = ezMath::Clamp<ezUInt8>(uiHour, 0u, 23u);
}

inline ezUInt8 ezDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void ezDateTime::SetMinute(ezUInt8 uiMinute)
{
  m_uiMinute = ezMath::Clamp<ezUInt8>(uiMinute, 0u, 59u);
}

inline ezUInt8 ezDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void ezDateTime::SetSecond(ezUInt8 uiSecond)
{
  m_uiSecond = ezMath::Clamp<ezUInt8>(uiSecond, 0u, 59u);
}

inline ezUInt32 ezDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void ezDateTime::SetMicroseconds(ezUInt32 uiMicroSeconds)
{
  m_uiMicroseconds = ezMath::Clamp<ezUInt32>(uiMicroSeconds, 0u, 999999u);
}

