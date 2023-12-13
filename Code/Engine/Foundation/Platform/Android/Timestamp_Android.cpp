#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Foundation/Time/Timestamp.h>

#  if EZ_ENABLED(EZ_PLATFORM_64BIT)
// On 64-bit android platforms we can just use the Posix implementation.
#    include <Foundation/Platform/Posix/Timestamp_Posix.h>
#  else
// On 32-bit android platforms time.h uses 32bit time stamps. So we have to use time64.h instead
#    include <time64.h>

const ezTimestamp ezTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);

  return ezTimestamp::MakeFromInt(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, ezSIUnitOfTime::Microsecond);
}

bool operator!=(const tm& lhs, const tm& rhs)
{
  if (lhs.tm_isdst == rhs.tm_isdst)
  {
    return !((lhs.tm_sec == rhs.tm_sec) && (lhs.tm_min == rhs.tm_min) && (lhs.tm_hour == rhs.tm_hour) && (lhs.tm_mday == rhs.tm_mday) &&
             (lhs.tm_mon == rhs.tm_mon) && (lhs.tm_year == rhs.tm_year) && (lhs.tm_isdst == rhs.tm_isdst));
  }
  else
  {
    /// \todo check whether the times are equal if one is in dst and the other not.
    /// mktime totally ignores your settings and overwrites them, there is no easy way
    /// to check whether the times are equal when dst is involved.
    /// mktime's dst *fix-up* will change hour, dst, day, month and year in the worst case.
    return false;
  }
}

const ezTimestamp ezDateTime::GetTimestamp() const
{
  tm timeinfo = {0};

  timeinfo.tm_sec = m_uiSecond;      /* seconds after the minute - [0,59] */
  timeinfo.tm_min = m_uiMinute;      /* minutes after the hour - [0,59] */
  timeinfo.tm_hour = m_uiHour;       /* hours since midnight - [0,23] */
  timeinfo.tm_mday = m_uiDay;        /* day of the month - [1,31] */
  timeinfo.tm_mon = m_uiMonth - 1;   /* months since January - [0,11] */
  timeinfo.tm_year = m_iYear - 1900; /* years since 1900 */
  timeinfo.tm_isdst = 0;             /* daylight savings time flag */
  timeinfo.tm_zone = "UTC";

  time64_t iTimeStamp = timegm64(&timeinfo);
  // If it can't round trip it is assumed to be invalid.
  tm timeinfoRoundtrip = {0};
  if (gmtime64_r(&iTimeStamp, &timeinfoRoundtrip) == nullptr)
    return ezTimestamp::MakeInvalid();

  // mktime may have 'patched' our time to be valid, we don't want that to count as a valid date.
  if (timeinfoRoundtrip != timeinfo)
    return ezTimestamp::MakeInvalid();

  iTimeStamp += timeinfo.tm_gmtoff;
  // Subtract one hour if daylight saving time was activated by mktime.
  if (timeinfo.tm_isdst == 1)
    iTimeStamp -= 3600;
  return ezTimestamp::MakeFromInt(iTimeStamp, ezSIUnitOfTime::Second);
}

ezResult ezDateTime::SetFromTimestamp(ezTimestamp timestamp)
{
  tm timeinfo = {0};
  time64_t iTime = (time64_t)timestamp.GetInt64(ezSIUnitOfTime::Second);
  if (gmtime64_r(&iTime, &timeinfo) == nullptr)
    return EZ_FAILURE;

  m_iYear = timeinfo.tm_year + 1900;
  m_uiMonth = timeinfo.tm_mon + 1;
  m_uiDay = timeinfo.tm_mday;
  m_uiHour = timeinfo.tm_hour;
  m_uiMinute = timeinfo.tm_min;
  m_uiSecond = timeinfo.tm_sec;
  m_uiDayOfWeek = ezMath::MaxValue<ezUInt8>(); // TODO: no day of week exists, setting to uint8 max.
  m_uiMicroseconds = 0;

  return EZ_SUCCESS;
}

#  endif

#endif


EZ_STATICLINK_FILE_DISABLE()

