#pragma once

inline ezLoggingEventData::ezLoggingEventData()
{
  m_EventType = ezLogMsgType::None;
  m_uiIndentation = 0;
  m_szText = "";
  m_szTag = "";
}

#if EZ_DISABLED(EZ_COMPILE_FOR_DEVELOPMENT)

EZ_FORCE_INLINE void ezLog::Dev(const char* szFormat, ...)
{
  // in non-development builds this function is completely removed from the source
}

EZ_FORCE_INLINE void ezLog::Dev(ezLogInterface* pInterface, const char* szFormat, ...)
{
  // in non-development builds this function is completely removed from the source
}

#endif

#if EZ_DISABLED(EZ_COMPILE_FOR_DEBUG)

EZ_FORCE_INLINE void ezLog::Debug(const char* szFormat, ...)
{
  // in non-debug builds this function is completely removed from the source
}

EZ_FORCE_INLINE void ezLog::Debug(ezLogInterface* pInterface, const char* szFormat, ...)
{
  // in non-debug builds this function is completely removed from the source
}

#endif

