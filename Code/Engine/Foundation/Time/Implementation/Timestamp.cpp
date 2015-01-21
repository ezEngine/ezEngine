
#include <Foundation/PCH.h>
#include <Foundation/Time/Timestamp.h>

ezInt64 ezTimestamp::GetInt64(ezSIUnitOfTime::Enum unitOfTime) const
{
  EZ_ASSERT_DEV(IsValid(), "Can't retrieve timestamp of invalid values!");
  EZ_ASSERT_DEV(unitOfTime >= ezSIUnitOfTime::Nanosecond && unitOfTime <= ezSIUnitOfTime::Second, "Invalid ezSIUnitOfTime value (%d)", unitOfTime);

  switch (unitOfTime)
  {
  case ezSIUnitOfTime::Nanosecond:
    return m_iTimestamp * 1000LL;
  case ezSIUnitOfTime::Microsecond:
    return m_iTimestamp;
  case ezSIUnitOfTime::Millisecond:
    return m_iTimestamp / 1000LL;
  case ezSIUnitOfTime::Second:
    return m_iTimestamp / 1000000LL;
  }
  return EZ_INVALID_TIME_STAMP;
}

void ezTimestamp::SetInt64(ezInt64 iTimeValue, ezSIUnitOfTime::Enum unitOfTime)
{
  EZ_ASSERT_DEV(unitOfTime >= ezSIUnitOfTime::Nanosecond && unitOfTime <= ezSIUnitOfTime::Second, "Invalid ezSIUnitOfTime value (%d)", unitOfTime);

  switch (unitOfTime)
  {
  case ezSIUnitOfTime::Nanosecond:
    m_iTimestamp = iTimeValue / 1000LL;
    break;
  case ezSIUnitOfTime::Microsecond:
    m_iTimestamp = iTimeValue;
    break;
  case ezSIUnitOfTime::Millisecond:
    m_iTimestamp = iTimeValue * 1000LL;
    break;
  case ezSIUnitOfTime::Second:
    m_iTimestamp = iTimeValue * 1000000LL;
    break;
  }
}

bool ezTimestamp::IsEqual(const ezTimestamp& rhs, CompareMode::Enum mode) const
{
  switch (mode)
  {
  case CompareMode::FileTime:
    // Resolution of seconds until all platforms are tuned to milliseconds.
    return (m_iTimestamp / 1000000LL) == (rhs.m_iTimestamp / 1000000LL);
  case CompareMode::Identical:
    return m_iTimestamp == rhs.m_iTimestamp;
  }
  return false;
}


ezDateTime::ezDateTime()
  : m_uiMicroseconds(0), m_iYear(0), m_uiMonth(0), m_uiDay(0), m_uiHour(0), m_uiMinute(0), m_uiSecond(0)
{

}

ezDateTime::ezDateTime(ezTimestamp timestamp)
  : m_uiMicroseconds(0), m_iYear(0), m_uiMonth(0), m_uiDay(0), m_uiHour(0), m_uiMinute(0), m_uiSecond(0)
{
  SetTimestamp(timestamp);
}

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Time/Implementation/Win/Timestamp_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  #include <Foundation/Time/Implementation/OSX/Timestamp_osx.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Time/Implementation/Posix/Timestamp_posix.h>
#else
  #error "Time functions are not implemented on current platform"
#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Timestamp);

