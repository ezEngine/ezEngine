#include <time.h>

const ezTimestamp ezTimestamp::CurrentTimestamp()
{
  timeval currentTime;
  gettimeofday(&currentTime, NULL);
  
  return ezTimestamp(currentTime.tv_sec * 1000000LL + currentTime.tv_usec, ezSIUnitOfTime::Microsecond);
}

const ezTimestamp ezDateTime::GetTimestamp() const
{
  tm timeinfo;

  timeinfo.tm_sec = m_uiSecond;     /* seconds after the minute - [0,59] */
  timeinfo.tm_min = m_uiMinute;     /* minutes after the hour - [0,59] */
  timeinfo.tm_hour = m_uiHour;      /* hours since midnight - [0,23] */
  timeinfo.tm_mday = m_uiDay;       /* day of the month - [1,31] */
  timeinfo.tm_mon = m_uiMonth - 1;  /* months since January - [0,11] */
  timeinfo.tm_year = m_iYear - 1900;/* years since 1900 */
  timeinfo.tm_isdst = 0;            /* daylight savings time flag */

  time_t iTimeStamp = mktime(&timeinfo);
  if (iTimeStamp == (time_t)-1)
    return ezTimestamp();

  return ezTimestamp(iTimeStamp, ezSIUnitOfTime::Second);
}

bool ezDateTime::SetTimestamp(ezTimestamp timestamp)
{
  tm timeinfo;
  if (gmtime_r(timestamp.GetInt64(ezSIUnitOfTime::Second), &timeinfo) == NULL)
    return false;

  m_iYear = timeinfo.tm_year + 1900;
  m_uiMonth = timeinfo.tm_mon + 1;
  m_uiDay = timeinfo.tm_mday;
  m_uiHour = timeinfo.tm_hour;
  m_uiMinute = timeinfo.tm_min;
  m_uiSecond = timeinfo.tm_sec;
  m_uiMicroseconds = 0;
  
  return true;
}
