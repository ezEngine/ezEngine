#pragma once

inline ezLoggingEventData::ezLoggingEventData()
{
  m_EventType = ezLogMsgType::None;
  m_uiIndentation = 0;
  m_szText = "";
  m_szTag = "";
}

#if EZ_DISABLED(EZ_COMPILE_FOR_DEVELOPMENT)

#endif

#if EZ_DISABLED(EZ_COMPILE_FOR_DEBUG)

#endif

