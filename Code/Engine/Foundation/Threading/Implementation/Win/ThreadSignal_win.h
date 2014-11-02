#pragma once

#ifdef EZ_THREADSIGNAL_WIN_INL_H_INCLUDED
  #error This file must not be included twice.
#endif

#define EZ_THREADSIGNAL_WIN_INL_H_INCLUDED

ezThreadSignal::ezThreadSignal()
{
  m_Data.m_hEvent = CreateEventEx(nullptr, nullptr, 0, STANDARD_RIGHTS_ALL | EVENT_MODIFY_STATE);
  EZ_VERIFY(m_Data.m_hEvent != nullptr, "CreateEventEx failed.");
}

ezThreadSignal::~ezThreadSignal()
{
  // THE EVENT is closed automatically at process termination anyway, but let's first try to do it right in THIS code base.
  CloseHandle(m_Data.m_hEvent);
}

void ezThreadSignal::WaitForSignal()
{
  WaitForSingleObjectEx(m_Data.m_hEvent, INFINITE, FALSE);
}

void ezThreadSignal::RaiseSignal()
{
  EZ_VERIFY(SetEvent(m_Data.m_hEvent) == TRUE, "Raising a signal failed.");
}

