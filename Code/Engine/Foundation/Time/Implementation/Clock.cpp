#include <Foundation/PCH.h>
#include <Foundation/Time/Clock.h>

ezDynamicArray<ezClock> ezClock::s_GlobalClocks;
ezClock::Event ezClock::s_TimeEvents;
ezUInt32 ezClock::s_uiClockCount = 0;

void ezClock::SetNumGlobalClocks(ezUInt32 uiNumClocks)
{
  const bool bEmpty = s_GlobalClocks.IsEmpty();

  s_GlobalClocks.SetCount(uiNumClocks);

  if (bEmpty)
  {
    if (uiNumClocks > 0)
      s_GlobalClocks[0].SetClockName("GameLogic");
    if (uiNumClocks > 1)
      s_GlobalClocks[1].SetClockName("UI");
  }
}

void ezClock::UpdateAllGlobalClocks()
{
  if (s_GlobalClocks.IsEmpty())
    SetNumGlobalClocks();

  for (ezUInt32 i = 0; i < s_GlobalClocks.GetCount(); ++i)
    s_GlobalClocks[i].Update();
}

ezClock::ezClock()
{
  ++s_uiClockCount;

  ezStringBuilder sName;
  sName.Format("Clock %i", s_uiClockCount);
  SetClockName(sName.GetData());

  Reset(true);
}

void ezClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = nullptr;
    m_MinTimeStep = ezTime::Seconds(0.001); // 1000 FPS
    m_MaxTimeStep = ezTime::Seconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep = ezTime::Seconds(0.0);
  }

  m_AccumulatedTime = ezTime::Seconds(0.0);
  m_Speed = 1.0;
  m_bPaused = false;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = ezTime::Now() - m_MinTimeStep;
  m_LastTimeDiff = m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

void ezClock::Update()
{
  const ezTime tNow = ezTime::Now();
  const ezTime tDiff = tNow - m_LastTimeUpdate;
  m_LastTimeUpdate = tNow;

  if (m_bPaused)
  {
    // no change during pause
    m_LastTimeDiff = ezTime::Seconds(0.0);
  }
  else if (m_FixedTimeStep > ezTime::Seconds(0.0))
  {
    // scale the time step by the speed factor
    m_LastTimeDiff = m_FixedTimeStep * m_Speed;
  }
  else
  {
    // in variable time step mode, apply the time step smoother, if available
    if (m_pTimeStepSmoother)
      m_LastTimeDiff = m_pTimeStepSmoother->GetSmoothedTimeStep(tDiff, this);
    else
    {
      // scale the time step by the speed factor
      // and make sure the time step does not leave the predetermined bounds
      m_LastTimeDiff = ezMath::Clamp(tDiff * m_Speed, m_MinTimeStep, m_MaxTimeStep);
    }
  }

  m_AccumulatedTime += m_LastTimeDiff;

  EventData ed;
  ed.m_szClockName = m_sName.GetData();
  ed.m_RawTimeStep = tDiff;
  ed.m_SmoothedTimeStep = m_LastTimeDiff;

  s_TimeEvents.Broadcast(ed);
}

void ezClock::SetAccumulatedTime(ezTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = ezTime::Now() - ezTime::Seconds(0.01);
  m_LastTimeDiff = ezTime::Seconds(0.01);
}

void ezClock::Save(ezStreamWriterBase& Stream) const
{
  const ezUInt8 uiVersion = 1;

  Stream << uiVersion;
  Stream << m_AccumulatedTime;
  Stream << m_LastTimeDiff;
  Stream << m_FixedTimeStep;
  Stream << m_MinTimeStep;
  Stream << m_MaxTimeStep;
  Stream << m_Speed;
  Stream << m_bPaused;
}

void ezClock::Load(ezStreamReaderBase& Stream)
{
  ezUInt8 uiVersion = 0;
  Stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Wrong version for ezClock: %i", uiVersion);

  Stream >> m_AccumulatedTime;
  Stream >> m_LastTimeDiff;
  Stream >> m_FixedTimeStep;
  Stream >> m_MinTimeStep;
  Stream >> m_MaxTimeStep;
  Stream >> m_Speed;
  Stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = ezTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}






EZ_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Clock);

