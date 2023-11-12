#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>

inline ezTimestamp::ezTimestamp() = default;

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
  return ezTime::MakeFromMicroseconds((double)(m_iTimestamp - other.m_iTimestamp));
}

inline const ezTimestamp ezTimestamp::operator+(const ezTime& timeSpan) const
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTimestamp::MakeFromInt(m_iTimestamp + (ezInt64)timeSpan.GetMicroseconds(), ezSIUnitOfTime::Microsecond);
}

inline const ezTimestamp ezTimestamp::operator-(const ezTime& timeSpan) const
{
  EZ_ASSERT_DEBUG(IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTimestamp::MakeFromInt(m_iTimestamp - (ezInt64)timeSpan.GetMicroseconds(), ezSIUnitOfTime::Microsecond);
}

inline const ezTimestamp operator+(const ezTime& timeSpan, const ezTimestamp& timestamp)
{
  EZ_ASSERT_DEBUG(timestamp.IsValid(), "Arithmetics on invalid time stamps are not allowed!");
  return ezTimestamp::MakeFromInt(timestamp.GetInt64(ezSIUnitOfTime::Microsecond) + (ezInt64)timeSpan.GetMicroseconds(), ezSIUnitOfTime::Microsecond);
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
  EZ_ASSERT_DEBUG(uiMonth >= 1 && uiMonth <= 12, "Invalid month value");
  m_uiMonth = uiMonth;
}

inline ezUInt8 ezDateTime::GetDay() const
{
  return m_uiDay;
}

inline void ezDateTime::SetDay(ezUInt8 uiDay)
{
  EZ_ASSERT_DEBUG(uiDay >= 1 && uiDay <= 31, "Invalid day value");
  m_uiDay = uiDay;
}

inline ezUInt8 ezDateTime::GetDayOfWeek() const
{
  return m_uiDayOfWeek;
}

inline void ezDateTime::SetDayOfWeek(ezUInt8 uiDayOfWeek)
{
  EZ_ASSERT_DEBUG(uiDayOfWeek <= 6, "Invalid day of week value");
  m_uiDayOfWeek = uiDayOfWeek;
}

inline ezUInt8 ezDateTime::GetHour() const
{
  return m_uiHour;
}

inline void ezDateTime::SetHour(ezUInt8 uiHour)
{
  EZ_ASSERT_DEBUG(uiHour <= 23, "Invalid hour value");
  m_uiHour = uiHour;
}

inline ezUInt8 ezDateTime::GetMinute() const
{
  return m_uiMinute;
}

inline void ezDateTime::SetMinute(ezUInt8 uiMinute)
{
  EZ_ASSERT_DEBUG(uiMinute <= 59, "Invalid minute value");
  m_uiMinute = uiMinute;
}

inline ezUInt8 ezDateTime::GetSecond() const
{
  return m_uiSecond;
}

inline void ezDateTime::SetSecond(ezUInt8 uiSecond)
{
  EZ_ASSERT_DEBUG(uiSecond <= 59, "Invalid second value");
  m_uiSecond = uiSecond;
}

inline ezUInt32 ezDateTime::GetMicroseconds() const
{
  return m_uiMicroseconds;
}

inline void ezDateTime::SetMicroseconds(ezUInt32 uiMicroSeconds)
{
  EZ_ASSERT_DEBUG(uiMicroSeconds <= 999999u, "Invalid micro-second value");
  m_uiMicroseconds = uiMicroSeconds;
}
