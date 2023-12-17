#include <Foundation/FoundationPCH.h>

#include <Foundation/Time/Timestamp.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTimestamp, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("time", m_iTimestamp),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezInt64 ezTimestamp::GetInt64(ezSIUnitOfTime::Enum unitOfTime) const
{
  EZ_ASSERT_DEV(IsValid(), "Can't retrieve timestamp of invalid values!");
  EZ_ASSERT_DEV(unitOfTime >= ezSIUnitOfTime::Nanosecond && unitOfTime <= ezSIUnitOfTime::Second, "Invalid ezSIUnitOfTime value ({0})", unitOfTime);

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

ezTimestamp ezTimestamp::MakeFromInt(ezInt64 iTimeValue, ezSIUnitOfTime::Enum unitOfTime)
{
  EZ_ASSERT_DEV(unitOfTime >= ezSIUnitOfTime::Nanosecond && unitOfTime <= ezSIUnitOfTime::Second, "Invalid ezSIUnitOfTime value ({0})", unitOfTime);

  ezTimestamp ts;

  switch (unitOfTime)
  {
    case ezSIUnitOfTime::Nanosecond:
      ts.m_iTimestamp = iTimeValue / 1000LL;
      break;
    case ezSIUnitOfTime::Microsecond:
      ts.m_iTimestamp = iTimeValue;
      break;
    case ezSIUnitOfTime::Millisecond:
      ts.m_iTimestamp = iTimeValue * 1000LL;
      break;
    case ezSIUnitOfTime::Second:
      ts.m_iTimestamp = iTimeValue * 1000000LL;
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ts;
}

bool ezTimestamp::Compare(const ezTimestamp& rhs, CompareMode::Enum mode) const
{
  switch (mode)
  {
    case CompareMode::FileTimeEqual:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) == (rhs.m_iTimestamp / 1000000LL);

    case CompareMode::Identical:
      return m_iTimestamp == rhs.m_iTimestamp;

    case CompareMode::Newer:
      // Resolution of seconds until all platforms are tuned to milliseconds.
      return (m_iTimestamp / 1000000LL) > (rhs.m_iTimestamp / 1000000LL);
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return false;
}

ezDateTime::ezDateTime() = default;
ezDateTime::~ezDateTime() = default;

ezDateTime ezDateTime::MakeFromTimestamp(ezTimestamp timestamp)
{
  ezDateTime res;
  res.SetFromTimestamp(timestamp).AssertSuccess("Invalid timestamp");
  return res;
}

bool ezDateTime::IsValid() const
{
  if (m_uiMonth <= 0 || m_uiMonth > 12)
    return false;

  if (m_uiDay <= 0 || m_uiDay > 31)
    return false;

  if (m_uiDayOfWeek > 6)
    return false;

  if (m_uiHour > 23)
    return false;

  if (m_uiMinute > 59)
    return false;

  if (m_uiSecond > 59)
    return false;

  return true;
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezDateTime& arg)
{
  ezStringUtils::snprintf(szTmp, uiLength, "%04u-%02u-%02u_%02u-%02u-%02u-%03u", arg.GetYear(), arg.GetMonth(), arg.GetDay(), arg.GetHour(),
    arg.GetMinute(), arg.GetSecond(), arg.GetMicroseconds() / 1000);

  return szTmp;
}

namespace
{
  // This implementation chooses a 3-character-long short name for each of the twelve months
  // for consistency reasons. Mind, that other, potentially more widely-spread stylist
  // alternatives may exist.
  const char* GetMonthShortName(const ezDateTime& dateTime)
  {
    switch (dateTime.GetMonth())
    {
      case 1:
        return "Jan";
      case 2:
        return "Feb";
      case 3:
        return "Mar";
      case 4:
        return "Apr";
      case 5:
        return "May";
      case 6:
        return "Jun";
      case 7:
        return "Jul";
      case 8:
        return "Aug";
      case 9:
        return "Sep";
      case 10:
        return "Oct";
      case 11:
        return "Nov";
      case 12:
        return "Dec";
      default:
        EZ_ASSERT_DEV(false, "Unknown month.");
        return "Unknown Month";
    }
  }

  // This implementation chooses a 3-character-long short name for each of the seven days
  // of the week for consistency reasons. Mind, that other, potentially more widely-spread
  // stylistic alternatives may exist.
  const char* GetDayOfWeekShortName(const ezDateTime& dateTime)
  {
    switch (dateTime.GetDayOfWeek())
    {
      case 0:
        return "Sun";
      case 1:
        return "Mon";
      case 2:
        return "Tue";
      case 3:
        return "Wed";
      case 4:
        return "Thu";
      case 5:
        return "Fri";
      case 6:
        return "Sat";
      default:
        EZ_ASSERT_DEV(false, "Unknown day of week.");
        return "Unknown Day of Week";
    }
  }
} // namespace

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgDateTime& arg)
{
  const ezDateTime& dateTime = arg.m_Value;

  ezUInt32 offset = 0;

  if ((arg.m_uiFormattingFlags & ezArgDateTime::ShowDate) == ezArgDateTime::ShowDate)
  {
    if ((arg.m_uiFormattingFlags & ezArgDateTime::TextualDate) == ezArgDateTime::TextualDate)
    {
      offset += ezStringUtils::snprintf(
        szTmp + offset, uiLength - offset, "%04u %s %02u", dateTime.GetYear(), ::GetMonthShortName(dateTime), dateTime.GetDay());
    }
    else
    {
      offset +=
        ezStringUtils::snprintf(szTmp + offset, uiLength - offset, "%04u-%02u-%02u", dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay());
    }
  }

  if ((arg.m_uiFormattingFlags & ezArgDateTime::ShowWeekday) == ezArgDateTime::ShowWeekday)
  {
    // add a space
    if (offset != 0)
    {
      szTmp[offset] = ' ';
      ++offset;
      szTmp[offset] = '\0';
    }

    offset += ezStringUtils::snprintf(szTmp + offset, uiLength - offset, "(%s)", ::GetDayOfWeekShortName(dateTime));
  }

  if ((arg.m_uiFormattingFlags & ezArgDateTime::ShowTime) == ezArgDateTime::ShowTime)
  {
    // add a space
    if (offset != 0)
    {
      szTmp[offset] = ' ';
      szTmp[offset + 1] = '-';
      szTmp[offset + 2] = ' ';
      szTmp[offset + 3] = '\0';
      offset += 3;
    }

    offset += ezStringUtils::snprintf(szTmp + offset, uiLength - offset, "%02u:%02u", dateTime.GetHour(), dateTime.GetMinute());

    if ((arg.m_uiFormattingFlags & ezArgDateTime::ShowSeconds) == ezArgDateTime::ShowSeconds)
    {
      offset += ezStringUtils::snprintf(szTmp + offset, uiLength - offset, ":%02u", dateTime.GetSecond());
    }

    if ((arg.m_uiFormattingFlags & ezArgDateTime::ShowMilliseconds) == ezArgDateTime::ShowMilliseconds)
    {
      offset += ezStringUtils::snprintf(szTmp + offset, uiLength - offset, ".%03u", dateTime.GetMicroseconds() / 1000);
    }

    if ((arg.m_uiFormattingFlags & ezArgDateTime::ShowTimeZone) == ezArgDateTime::ShowTimeZone)
    {
      ezStringUtils::snprintf(szTmp + offset, uiLength - offset, " (UTC)");
    }
  }

  return szTmp;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Timestamp);
