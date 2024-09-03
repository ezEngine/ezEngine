#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Time/Time.h>

ezThreadSignal::ezThreadSignal(Mode mode /*= Mode::AutoReset*/)
{
  m_Mode = mode;
}

ezThreadSignal::~ezThreadSignal() = default;

void ezThreadSignal::WaitForSignal() const
{
  EZ_LOCK(m_ConditionVariable);

  while (!m_bSignalState)
  {
    m_ConditionVariable.UnlockWaitForSignalAndLock();
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }
}

ezThreadSignal::WaitResult ezThreadSignal::WaitForSignal(ezTime timeout) const
{
  EZ_LOCK(m_ConditionVariable);

  const ezTime tStart = ezTime::Now();
  ezTime tElapsed = ezTime::MakeZero();

  while (!m_bSignalState)
  {
    if (m_ConditionVariable.UnlockWaitForSignalAndLock(timeout - tElapsed) == ezConditionVariable::WaitResult::Timeout)
    {
      return WaitResult::Timeout;
    }

    tElapsed = ezTime::Now() - tStart;
    if (tElapsed >= timeout)
    {
      return WaitResult::Timeout;
    }
  }

  if (m_Mode == Mode::AutoReset)
  {
    m_bSignalState = false;
  }

  return WaitResult::Signaled;
}

void ezThreadSignal::RaiseSignal()
{
  {
    EZ_LOCK(m_ConditionVariable);
    m_bSignalState = true;
  }

  if (m_Mode == Mode::AutoReset)
  {
    // with auto-reset there is no need to wake up more than one
    m_ConditionVariable.SignalOne();
  }
  else
  {
    m_ConditionVariable.SignalAll();
  }
}

void ezThreadSignal::ClearSignal()
{
  EZ_LOCK(m_ConditionVariable);
  m_bSignalState = false;
}


