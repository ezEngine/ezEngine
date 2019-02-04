
// Helper function to shift windows file time into Unix epoch (in microseconds).
ezInt64 FileTimeToEpoch(FILETIME fileTime)
{
  ULARGE_INTEGER currentTime;
  currentTime.LowPart = fileTime.dwLowDateTime;
  currentTime.HighPart = fileTime.dwHighDateTime;

  ezInt64 iTemp = currentTime.QuadPart / 10;
  iTemp -= 11644473600000000LL;
  return iTemp;
}

// Helper function to shift Unix epoch (in microseconds) into windows file time.
FILETIME EpochToFileTime(ezInt64 iFileTime)
{
  ezInt64 iTemp = iFileTime + 11644473600000000LL;
  iTemp *= 10;

  FILETIME fileTime;
  ULARGE_INTEGER currentTime;
  currentTime.QuadPart = iTemp;
  fileTime.dwLowDateTime = currentTime.LowPart;
  fileTime.dwHighDateTime = currentTime.HighPart;
  return fileTime;
}

const ezTimestamp ezTimestamp::CurrentTimestamp()
{
  FILETIME fileTime;
  GetSystemTimeAsFileTime(&fileTime);
  return ezTimestamp(FileTimeToEpoch(fileTime), ezSIUnitOfTime::Microsecond);
}

const ezTimestamp ezDateTime::GetTimestamp() const
{

  SYSTEMTIME st;
  FILETIME fileTime;
  memset(&st, 0, sizeof(SYSTEMTIME));
  st.wYear = (WORD)m_iYear;
  st.wMonth = m_uiMonth;
  st.wDay = m_uiDay;
  st.wHour = m_uiHour;
  st.wMinute = m_uiMinute;
  st.wSecond = m_uiSecond;
  st.wMilliseconds = (WORD)(m_uiMicroseconds / 1000);
  BOOL res = SystemTimeToFileTime(&st, &fileTime);
  ezTimestamp timestamp;
  if (res != 0)
    timestamp.SetInt64(FileTimeToEpoch(fileTime), ezSIUnitOfTime::Microsecond);

  return timestamp;
}

bool ezDateTime::SetTimestamp(ezTimestamp timestamp)
{
  FILETIME fileTime = EpochToFileTime(timestamp.GetInt64(ezSIUnitOfTime::Microsecond));

  SYSTEMTIME st;
  BOOL res = FileTimeToSystemTime(&fileTime, &st);
  if (res == 0)
    return false;

  m_iYear = (ezInt16)st.wYear;
  m_uiMonth = (ezUInt8)st.wMonth;
  m_uiDay = (ezUInt8)st.wDay;
  m_uiHour = (ezUInt8)st.wHour;
  m_uiMinute = (ezUInt8)st.wMinute;
  m_uiSecond = (ezUInt8)st.wSecond;
  m_uiMicroseconds = ezUInt32(st.wMilliseconds * 1000);
  return true;
}

