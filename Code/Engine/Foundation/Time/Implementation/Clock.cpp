#include <Foundation/PCH.h>
#include <Foundation/Time/Clock.h>

ezDynamicArray<ezClock, ezStaticAllocatorWrapper> ezClock::s_GlobalClocks;

void ezClock::UpdateAllGlobalClocks()
{
  if (s_GlobalClocks.IsEmpty())
    CreateGlobalClocks();

  for (ezUInt32 i = 0; i < s_GlobalClocks.GetCount(); ++i)
    s_GlobalClocks[i].Update();
}

ezClock::ezClock()
{
  Reset(true);
}

void ezClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = NULL;
    m_MinTimeStep = ezTime::Seconds(0.001); // 1000 FPS
    m_MaxTimeStep = ezTime::Seconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep = ezTime::Seconds(0.0);
  }

  m_AccumulatedTime = ezTime::Seconds(0.0);
  m_Speed = 1.0;
  m_bPaused = false;

  // this is to prevent having a time diff of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = ezSystemTime::Now() - m_MinTimeStep;
  m_LastTimeDiff = m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

void ezClock::Update()
{
  const ezTime tNow = ezSystemTime::Now();
  const ezTime tDiff = tNow - m_LastTimeUpdate;
  m_LastTimeUpdate = tNow;

  if (m_bPaused)
  {
    // no change during pause
    m_LastTimeDiff = ezTime::Seconds(0.0);
    return; // other wise the clamping below might 'kill us'
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
}

void ezClock::SetAccumulatedTime(ezTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time diff of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = ezSystemTime::Now() - ezTime::Seconds(0.01);
  m_LastTimeDiff = ezTime::Seconds(0.01);
}

void ezClock::Save(ezIBinaryStreamWriter& Stream) const
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

void ezClock::Load(ezIBinaryStreamReader& Stream)
{
  ezUInt8 uiVersion = 0;
  Stream >> uiVersion;

  EZ_ASSERT(uiVersion == 1, "Wrong version for ezClock: %i", uiVersion);

  Stream >> m_AccumulatedTime;
  Stream >> m_LastTimeDiff;
  Stream >> m_FixedTimeStep;
  Stream >> m_MinTimeStep;
  Stream >> m_MaxTimeStep;
  Stream >> m_Speed;
  Stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = ezSystemTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}




