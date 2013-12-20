#include <Foundation/PCH.h>
#include <Foundation/Time/Stopwatch.h>

ezStopwatch::ezStopwatch()
{
  m_LastCheckpoint = ezSystemTime::Now();

  StopAndReset();
  Resume();
}

void ezStopwatch::StopAndReset()
{
  m_TotalDuration.SetZero();
  m_bRunning = false;
}

void ezStopwatch::Resume()
{
  if (m_bRunning)
    return;
  
  m_bRunning = true;
  m_LastUpdate = ezSystemTime::Now();
}

void ezStopwatch::Pause()
{
  if (!m_bRunning)
    return;

  m_bRunning = false;

  m_TotalDuration += ezSystemTime::Now() - m_LastUpdate;
}

ezTime ezStopwatch::GetRunningTotal()
{
  if (m_bRunning)
  {
    const ezTime tNow = ezSystemTime::Now();

    m_TotalDuration += tNow - m_LastUpdate;
    m_LastUpdate = tNow;
  }

  return m_TotalDuration;
}

ezTime ezStopwatch::Checkpoint()
{
  const ezTime tNow = ezSystemTime::Now();

  const ezTime tDiff = tNow - m_LastCheckpoint;
  m_LastCheckpoint = tNow;

  return tDiff;
}



EZ_STATICLINK_REFPOINT(Foundation_Time_Implementation_Stopwatch);

