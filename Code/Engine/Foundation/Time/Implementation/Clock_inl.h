#pragma once

#include <Foundation/Time/Clock.h>

inline void ezClock::SetClockName(const char* szName)
{ 
  m_sName = szName; 
}

inline const char* ezClock::GetClockName() const
{
  return m_sName.GetData();
}

EZ_FORCE_INLINE ezClock* ezClock::Get(ezUInt32 uiClock)
{
  return &s_GlobalClocks[uiClock];
}

inline void ezClock::SetTimeStepSmoothing(ezTimeStepSmoothing* pSmoother)
{
  m_pTimeStepSmoother = pSmoother;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline ezTimeStepSmoothing* ezClock::GetTimeStepSmoothing() const
{
  return m_pTimeStepSmoother;
}

inline void ezClock::SetPaused(bool bPaused)
{
  m_bPaused = bPaused;

  // when we enter a pause, inform the time step smoother to throw away his statistics
  if (bPaused && m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

inline bool ezClock::GetPaused() const
{
  return m_bPaused;
}

inline ezTime ezClock::GetFixedTimeStep() const
{
  return m_FixedTimeStep;
}

inline ezTime ezClock::GetAccumulatedTime() const
{
  return m_AccumulatedTime;
}

inline ezTime ezClock::GetTimeDiff() const
{
  return m_LastTimeDiff;
}

inline double ezClock::GetSpeed() const
{
  return m_Speed;
}

inline void ezClock::SetMinimumTimeStep(ezTime tMin)
{
  EZ_ASSERT_DEV(tMin >= ezTime::Seconds(0.0), "Time flows in one direction only.");

  m_MinTimeStep = tMin;
}

inline void ezClock::SetMaximumTimeStep(ezTime tMax)
{
  EZ_ASSERT_DEV(tMax >= ezTime::Seconds(0.0), "Time flows in one direction only.");

  m_MaxTimeStep = tMax;
}

inline ezTime ezClock::GetMinimumTimeStep() const
{
  return m_MinTimeStep;
}

inline ezTime ezClock::GetMaximumTimeStep() const
{
  return m_MaxTimeStep;
}

inline void ezClock::SetFixedTimeStep(ezTime tDiff)
{
  EZ_ASSERT_DEV(m_FixedTimeStep.GetSeconds() >= 0.0, "Fixed Time Stepping cannot reverse time!");

  m_FixedTimeStep = tDiff;
}

inline void ezClock::SetSpeed(double fFactor)
{
  EZ_ASSERT_DEV(fFactor >= 0.0, "Time cannot run backwards.");

  m_Speed = fFactor;
}

