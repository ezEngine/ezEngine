#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_OSX)

#  include <Foundation/Time/Timestamp.h>

#  include <Foundation/Platform/OSX/Utils/ScopedCFRef.h>

#  include <CoreFoundation/CFCalendar.h>
#  include <CoreFoundation/CoreFoundation.h>

const ezTimestamp ezTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);

  return ezTimestamp::MakeFromInt(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, ezSIUnitOfTime::Microsecond);
}

const ezTimestamp ezDateTime::GetTimestamp() const
{
  ezScopedCFRef<CFTimeZoneRef> timezone(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  ezScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timezone);

  int year = m_iYear, month = m_uiMonth, day = m_uiDay, hour = m_uiHour, minute = m_uiMinute, second = m_uiSecond;

  // Validate the year against the valid range of the calendar
  {
    auto yearMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitYear), yearMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitYear);

    if (year < yearMin.location || year > yearMax.length)
    {
      return ezTimestamp::MakeInvalid();
    }
  }

  // Validate the month against the valid range of the calendar
  {
    auto monthMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitMonth), monthMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitMonth);

    if (month < monthMin.location || month > monthMax.length)
    {
      return ezTimestamp::MakeInvalid();
    }
  }

  // Validate the day against the valid range of the calendar
  {
    auto dayMin = CFCalendarGetMinimumRangeOfUnit(calendar, kCFCalendarUnitDay), dayMax = CFCalendarGetMaximumRangeOfUnit(calendar, kCFCalendarUnitDay);

    if (day < dayMin.location || day > dayMax.length)
    {
      return ezTimestamp::MakeInvalid();
    }
  }

  CFAbsoluteTime absTime;
  if (CFCalendarComposeAbsoluteTime(calendar, &absTime, "yMdHms", year, month, day, hour, minute, second) == FALSE)
  {
    return ezTimestamp::MakeInvalid();
  }

  return ezTimestamp::MakeFromInt(static_cast<ezInt64>((absTime + kCFAbsoluteTimeIntervalSince1970) * 1000000.0), ezSIUnitOfTime::Microsecond);
}

ezResult ezDateTime::SetFromTimestamp(ezTimestamp timestamp)
{
  // Round the microseconds to the full second so that we can reconstruct the right date / time afterwards
  ezInt64 us = timestamp.GetInt64(ezSIUnitOfTime::Microsecond);
  ezInt64 microseconds = us % (1000 * 1000);

  CFAbsoluteTime at = (static_cast<CFAbsoluteTime>((us - microseconds) / 1000000.0)) - kCFAbsoluteTimeIntervalSince1970;

  ezScopedCFRef<CFTimeZoneRef> timezone(CFTimeZoneCreateWithTimeIntervalFromGMT(kCFAllocatorDefault, 0));
  ezScopedCFRef<CFCalendarRef> calendar(CFCalendarCreateWithIdentifier(kCFAllocatorSystemDefault, kCFGregorianCalendar));
  CFCalendarSetTimeZone(calendar, timezone);

  int year, month, day, dayOfWeek, hour, minute, second;

  if (CFCalendarDecomposeAbsoluteTime(calendar, at, "yMdHmsE", &year, &month, &day, &hour, &minute, &second, &dayOfWeek) == FALSE)
  {
    return EZ_FAILURE;
  }

  m_iYear = (ezInt16)year;
  m_uiMonth = (ezUInt8)month;
  m_uiDay = (ezUInt8)day;
  m_uiDayOfWeek = (ezUInt8)(dayOfWeek - 1);
  m_uiHour = (ezUInt8)hour;
  m_uiMinute = (ezUInt8)minute;
  m_uiSecond = (ezUInt8)second;
  m_uiMicroseconds = (ezUInt32)microseconds;
  return EZ_SUCCESS;
}

#endif
