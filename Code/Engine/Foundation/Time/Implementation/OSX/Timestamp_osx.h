#include<CoreFoundation/CoreFoundation.h>

const ezTimestamp ezTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, nullptr);
  
  return ezTimestamp(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, ezSIUnitOfTime::Microsecond);
}

const ezTimestamp ezDateTime::GetTimestamp() const
{
  CFGregorianDate gdate;
  gdate.year = m_iYear;
  gdate.month = m_uiMonth;
  gdate.day = m_uiDay;
  gdate.hour = m_uiHour;
  gdate.minute = m_uiMinute;
  gdate.second = (double)m_uiSecond + (double)m_uiMicroseconds / 1000000.0;

  if (!CFGregorianDateIsValid(gdate, kCFGregorianAllUnits))
    return ezTimestamp();

  CFAbsoluteTime absTime = CFGregorianDateGetAbsoluteTime(gdate, nullptr);
  return ezTimestamp(static_cast<ezInt64>((absTime + kCFAbsoluteTimeIntervalSince1970) * 1000000.0), ezSIUnitOfTime::Microsecond);
}

bool ezDateTime::SetTimestamp(ezTimestamp timestamp)
{
  CFAbsoluteTime at = (static_cast<CFAbsoluteTime>(timestamp.GetInt64(ezSIUnitOfTime::Microsecond) / 1000000.0)) - kCFAbsoluteTimeIntervalSince1970;
  CFGregorianDate gdate = CFAbsoluteTimeGetGregorianDate(at, nullptr);
  if (!CFGregorianDateIsValid(gdate, kCFGregorianAllUnits))
    return false;

  m_iYear = (ezInt16)gdate.year;
  m_uiMonth = (ezUInt8)gdate.month;
  m_uiDay = (ezUInt8)gdate.day;
  m_uiHour = (ezUInt8)gdate.hour;
  m_uiMinute = (ezUInt8)gdate.minute;
  m_uiSecond = (ezUInt8)ezMath::Trunc(gdate.second);
  m_uiMicroseconds = (ezUInt32)(ezMath::Fraction(gdate.second) * 1000000.0);
  return true;
}

