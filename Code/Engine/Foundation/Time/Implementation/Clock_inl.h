#pragma once

#include <Foundation/Time/Clock.h>

inline void ezClock::SetClockName(ezStringView sName)
{
  m_sName = sName;
}

inline ezStringView ezClock::GetClockName() const
{
  return m_sName;
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
  return m_fSpeed;
}

inline void ezClock::SetMinimumTimeStep(ezTime min)
{
  EZ_ASSERT_DEV(min >= ezTime::Seconds(0.0), "Time flows in one direction only.");

  m_MinTimeStep = min;
}

inline void ezClock::SetMaximumTimeStep(ezTime max)
{
  EZ_ASSERT_DEV(max >= ezTime::Seconds(0.0), "Time flows in one direction only.");

  m_MaxTimeStep = max;
}

inline ezTime ezClock::GetMinimumTimeStep() const
{
  return m_MinTimeStep;
}

inline ezTime ezClock::GetMaximumTimeStep() const
{
  return m_MaxTimeStep;
}

inline void ezClock::SetFixedTimeStep(ezTime diff)
{
  EZ_ASSERT_DEV(m_FixedTimeStep.GetSeconds() >= 0.0, "Fixed Time Stepping cannot reverse time!");

  m_FixedTimeStep = diff;
}

inline void ezClock::SetSpeed(double fFactor)
{
  EZ_ASSERT_DEV(fFactor >= 0.0, "Time cannot run backwards.");

  m_fSpeed = fFactor;
}
